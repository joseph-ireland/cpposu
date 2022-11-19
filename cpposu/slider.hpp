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
    SliderTick legacyLastTick;
};

struct LegacyLastTickEvent
{
    double distance, time;


    LegacyLastTickEvent(const slider_data& data, double tick_distance, double tick_duration)
    {
        constexpr double legacy_last_tick_offset = 36.0;
        double spanDuration = data.length * tick_duration/tick_distance;
        int finalSpanIndex = data.slide_count - 1;
        double finalSpanStartTime = finalSpanIndex * spanDuration;
        double totalDuration = data.slide_count * spanDuration;
        double finalSpanEndTime = std::max(totalDuration / 2, (finalSpanStartTime + spanDuration) - legacy_last_tick_offset);
        double finalProgress = std::clamp((finalSpanEndTime - finalSpanStartTime) / spanDuration, 0.0, 1.0);
        if (data.slide_count % 2 == 0) finalProgress = 1 - finalProgress;

        double endTimeMin = finalSpanEndTime / spanDuration;
        if (std::fmod(endTimeMin, 2) >= 1)
            endTimeMin = 1 - std::fmod(endTimeMin, 1);
        else
            endTimeMin = std::fmod(endTimeMin, 1);


        distance = endTimeMin*data.length;
        time = finalSpanEndTime;
    }
};
inline SliderPathTicks calculate_ticks(const std::vector<Vector2>& path, const slider_data& data, double tick_distance, double tick_duration)
{
    SliderPathTicks result;
    result.ticks.push_back({0, path[0]});

    const double min_distance_from_end = 10 * tick_distance/tick_duration;
    const double slider_length = data.length;

    const LegacyLastTickEvent legacy_last_tick(data,tick_distance,tick_duration);


    double nextTick = tick_distance;
    double nextTime = tick_duration;

    bool end = false;
    bool have_legacy_last_tick = false;

    auto check_slider_end = [&] {
        if (nextTick + min_distance_from_end >= slider_length)
        {
            nextTick = slider_length;
            nextTime = data.length * tick_duration/tick_distance;
            end = true;
        }
    };

    check_slider_end();


    double currentLength=0;
    for (int i=1, last=path.size()-1; i<=last; ++i)
    {
        Vector2 currentPoint = path[i-1];
        Vector2 nextPoint = path[i];
        double length = (nextPoint-currentPoint).length();
        double nextLength = currentLength+length;

        auto compute_tick = [&](double time, double distance) {
            if (length < 1e-6)
                return SliderTick{time, currentPoint};
            else
            {
                float t = (distance - currentLength)/length;
                return SliderTick{time, lerp(currentPoint, nextPoint, t)};
            }
        };
        if (!have_legacy_last_tick && (legacy_last_tick.distance <= nextLength || i == last))
        {
            result.legacyLastTick = compute_tick(legacy_last_tick.time, legacy_last_tick.distance);
            have_legacy_last_tick = true;
        }

        while(nextLength > nextTick || i == last)
        {

            result.ticks.push_back(compute_tick(nextTime, nextTick));

            if (end)
            {
                if (!have_legacy_last_tick) throw std::runtime_error("legacy last tick past end of slider");
                return result;
            }

            nextTick += tick_distance;
            nextTime += tick_duration;

            check_slider_end();
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

    return calculate_ticks(path, data, tick_distance, tick_duration);
}
inline SliderPathTicks calculate_centripetal_catmull_rom_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    std::vector<Vector2> path = ApproximateCatmull({data.control_points.begin(), data.control_points.end()});
    return calculate_ticks(path, data, tick_distance, tick_duration);
}
inline SliderPathTicks calculate_linear_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    return calculate_ticks(data.control_points, data, tick_distance, tick_duration);
}

inline SliderPathTicks calculate_circle_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    auto arc = CircularArc::fromControlPoints(data.control_points);
    if (!arc) return calculate_bezier_ticks(data, tick_distance, tick_duration);

    SliderPathTicks result;
    result.ticks.push_back({0,data.control_points[0]});

    double distance = tick_distance;
    double time=tick_duration;
    const double min_distance_from_end = 10 * tick_distance/tick_duration;


    while (distance + min_distance_from_end < data.length)
    {
        result.ticks.push_back({time, arc->value_at(distance)});
        distance += tick_distance;
        time += tick_duration;
    }
    result.ticks.push_back({data.length * tick_duration/tick_distance, arc->value_at(data.length)});

    const LegacyLastTickEvent legacy_last_tick(data,tick_distance,tick_duration);
    result.legacyLastTick = {legacy_last_tick.time, arc->value_at(legacy_last_tick.distance)};
    return result;
}

inline SliderPathTicks calculate_ticks(const slider_data& data, double tick_distance, double tick_duration)
{
    if (tick_duration == 0 || tick_distance == 0)
    {
        return {};
    }
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

    if (tick_locations.ticks.empty())
    {
        on_event(HitObject{
            .type = HitObjectType::slider_legacy_last_tick,
            .x = data.slider_head.x,
            .y = data.slider_head.y,
            .time = data.slider_head.time,
        });
        on_event(HitObject{
            .type = HitObjectType::slider_tail,
            .x = data.slider_head.x,
            .y = data.slider_head.y,
            .time = data.slider_head.time,
        });
        return;
    }

    for (int repeat = 0; repeat < data.slide_count; repeat+=2)
    {
        double slide_start = data.slider_head.time + repeat*slide_duration;
        for (int i=1; i<ticks.size(); ++i)
        {
            if (i!=ticks.size()-1 || repeat!=data.slide_count-1)
                on_event(HitObject{
                    .type = HitObjectType::slider_tick,
                    .x = ticks[i].position.X,
                    .y = ticks[i].position.Y,
                    .time = slide_start + ticks[i].time,
                });
            else
            {
                on_event(HitObject{
                    .type = HitObjectType::slider_legacy_last_tick,
                    .x = tick_locations.legacyLastTick.position.X,
                    .y = tick_locations.legacyLastTick.position.Y,
                    .time = data.slider_head.time + tick_locations.legacyLastTick.time,
                });
                on_event(HitObject{
                    .type = HitObjectType::slider_tail,
                    .x = ticks[i].position.X,
                    .y = ticks[i].position.Y,
                    .time = slide_start + ticks[i].time,
                });
            }
        }
        double slide_end = data.slider_head.time + (repeat+2)*slide_duration;

        if (repeat+1 < data.slide_count)
        {
            for (int i=ticks.size()-2; i>=0; --i)
            {
                if (i!=0 || repeat+1!=data.slide_count-1)
                    on_event(HitObject{
                        .type = HitObjectType::slider_tick,
                        .x = ticks[i].position.X,
                        .y = ticks[i].position.Y,
                        .time = slide_end - ticks[i].time,
                    });
                else
                {
                    on_event(HitObject{
                        .type = HitObjectType::slider_legacy_last_tick,
                        .x = tick_locations.legacyLastTick.position.X,
                        .y = tick_locations.legacyLastTick.position.Y,
                        .time = data.slider_head.time + tick_locations.legacyLastTick.time,
                    });
                    on_event(HitObject{
                        .type = HitObjectType::slider_tail,
                        .x = ticks[i].position.X,
                        .y = ticks[i].position.Y,
                        .time = slide_end - ticks[i].time,
                    });
                }
            }
        }
    }
}


}
