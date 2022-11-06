#pragma once

#include "cpposu/path.hpp"
#include "cpposu/slider.hpp"
#include <cmath>
#include <cpposu/types.hpp>
#include <cpposu/line_parser.hpp>

namespace cpposu {


struct control_point
{
    double x,y;
};

enum class slider_type
{
    Bezier='B',
    CentripetalCatmullRom='C',
    Linear='L',
    PerfectCircle='P',
};

inline std::optional<slider_type> try_parse_slider_type(std::string_view s)
{
    if (s.size() != 1) return {};
    slider_type result{s[0]};
    switch(result)
    {
        case slider_type::Bezier:
        case slider_type::CentripetalCatmullRom:
        case slider_type::Linear:
        case slider_type::PerfectCircle:
            return result;
        default:
            return {};
    }
}

struct slider_data
{
    HitObject slider_head;
    slider_type type;
    std::vector<Vector2> control_points;
    int slide_count;
    double length;
};


struct SliderTick
{
    double time;
    Vector2 position;
};

struct SliderPathTicks
{
    std::vector<SliderTick> ticks;
};


inline SliderPathTicks calculate_ticks(const std::vector<Vector2>& path, double slider_length, double tick_distance, double tick_duration)
{
    SliderPathTicks result;
    result.ticks.push_back({0, path[0]});
    float nextTick = tick_distance;
    double nextTime=tick_duration;

    float currentLength=0;
    bool sliderEnd = false;
    for (int i=1, last=path.size()-1; i<=last; ++i)
    {
        Vector2 currentPoint = path[i-1];
        Vector2 nextPoint = path[i];
        float length = (nextPoint-currentPoint).length();
        float nextLength = currentLength+length;

        while(nextLength > nextTick || i == last)
        {
            if (length < 1e-6) // avoid numerical instability
                result.ticks.push_back({nextTime, currentPoint}); 
            else
            {
                float t = (nextTick - currentLength)/length;
                result.ticks.push_back({nextTime, lerp(path[i-1], path[i], t)});
            }
            if (sliderEnd) return result; 

            nextTick += tick_distance;
            nextTime += tick_duration;

            if (nextTick + 1e-6 > slider_length)  // TODO check if game has an epsilon here
            {
                nextTick = slider_length;
                nextTime = slider_length * tick_duration/tick_distance;
                sliderEnd = true;
            }
        }
        currentLength = nextLength;
    }
    throw std::runtime_error("trying to get ticks from empty path");
}

inline SliderPathTicks calculate_bezier_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    std::vector<Vector2> path;
    auto begin = data.control_points.begin();
    auto end = data.control_points.end();
    auto next = begin;
    while(true)
    {
        auto current = next;
        ++next;
        if (next==end || *current == *(next)) // reached final point, or reached repeated point
        {
            ApproximateBezier(path, {begin, next});
            if (next==end)
                break;
            else
                begin=next;
        }
    }

    return calculate_ticks(path, data.length, tick_distance, tick_duration);
}
inline SliderPathTicks calculate_centripetal_catmull_rom_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    std::vector<Vector2> path = ApproximateCatmull({data.control_points.begin(), data.control_points.end()});
    return calculate_ticks(path, data.length, tick_distance, tick_duration);
}
inline SliderPathTicks calculate_linear_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    return calculate_ticks(data.control_points, data.length, tick_distance, tick_duration);
}

inline SliderPathTicks calculate_circle_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    auto arc = CircularArc::fromControlPoints(data.control_points);
    if (!arc) return calculate_bezier_ticks(data, tick_distance, tick_duration);

    SliderPathTicks result;
    result.ticks.push_back({0,data.control_points[0]});

    float distance = tick_distance;
    double time=tick_duration;

    while (distance + 1e-6 < data.length)
    {
        result.ticks.push_back({time, arc->value_at(distance)});
        distance += tick_distance;
        time += tick_duration;
    }
    result.ticks.push_back({data.length * tick_duration/tick_distance, arc->value_at(data.length)});

    return result; 
}

inline SliderPathTicks calculate_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    switch(data.type)
    {
        case slider_type::Bezier:
            return calculate_bezier_ticks(data, tick_distance, tick_duration);
        case slider_type::CentripetalCatmullRom:
            return calculate_centripetal_catmull_rom_ticks(data, tick_distance, tick_duration);
        case slider_type::Linear:
            return calculate_linear_ticks(data, tick_distance, tick_duration);
        case slider_type::PerfectCircle:
            return calculate_circle_ticks(data, tick_distance, tick_duration);
    }
}

template <typename OnEvent>
void generate_slider_hit_objects(const slider_data& data, TimingPoints& timing_points, OnEvent&& on_event)
{
    // TODO - LegacyLastTick

    timing_points.advanceTime(data.slider_head.time);
    double tick_distance = timing_points.tickDistance();
    double tick_duration = timing_points.tickDuration();
    double slide_duration = tick_duration * data.length / tick_distance;

    SliderPathTicks tick_locations = calculate_ticks(data,timing_points.tickDistance(), timing_points.tickDuration());
    const auto& ticks = tick_locations.ticks; 
    HitObject obj = data.slider_head;
    int tick_index = 0;

    on_event(data.slider_head);

    for (int repeat = 0; repeat < data.slide_count; repeat+=2)
    {
        double slide_start = data.slider_head.time + repeat*slide_duration;
        for (int i=1; i<ticks.size(); ++i)
        {
            on_event(HitObject{
                .type = (i==ticks.size()-1 && repeat==data.slide_count-1) ? HitObjectType::slider_tail : HitObjectType::slider_tick,
                .x = ticks[i].position.X,
                .y = ticks[i].position.Y,
                .time = float(slide_start + ticks[i].time),
            });
        }
        double slide_end = data.slider_head.time + (repeat+2)*slide_duration;

        if (repeat+1 < data.slide_count)
        {
            for (int i=ticks.size()-2; i>=0; --i)
            {
                on_event(HitObject{
                    .type = (i==0 && repeat+1==data.slide_count-1) ? HitObjectType::slider_tail : HitObjectType::slider_tick,
                    .x = ticks[i].position.X,
                    .y = ticks[i].position.Y,
                    .time = float(slide_end - ticks[i].time),
                });
            }
        }
    }
}


}