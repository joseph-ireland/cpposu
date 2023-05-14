#pragma once



#include <cpposu/path.hpp>
#include <cpposu/types.hpp>

#include <cmath>

namespace cpposu {



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
    std::vector<SliderControlPoint> control_points;
    int slide_count;
    double length;
};

struct SliderTick
{
    double time;
    Vector2 position;
};

struct LegacyLastTickEvent
{
    double distance, time;

};

struct Slider
{
    slider_data data;

    template <typename OnEvent>
    void generate_hit_objects(TimingPoints& timing_points, int beatmap_version, OnEvent&& on_event)
    {

        path.clear();
        cumulative_distance.clear();
        ticks.clear();

        timing_points.advanceTime(data.slider_head.time);
        tick_distance = timing_points.tickDistance(beatmap_version);
        tick_duration = timing_points.tickDuration(beatmap_version);

        if (data.control_points.size()==1 || tick_distance == 0)
        {
            on_event(data.slider_head);
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

        calculate_path();
        calculate_distances();
        calculate_ticks();

        double slide_duration = tick_duration * path_length / tick_distance;

        HitObject obj = data.slider_head;
        int tick_index = 0;

        on_event(data.slider_head);

        if (ticks.empty())
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
            double slide_start = repeat*slide_duration;
            for (int i=1; i<ticks.size(); ++i)
            {
                double time = slide_start + ticks[i].time;
                if (i!=ticks.size()-1 || repeat!=data.slide_count-1)
                    on_event(make_hit_object( HitObjectType::slider_tick, i, time));
                else
                {
                    on_event(make_legacy_last_tick_object(slide_start + ticks[i-1].time));
                    on_event(make_hit_object( HitObjectType::slider_tail, i, time));
                }
            }
            double slide_end = (repeat+2)*slide_duration;

            if (repeat+1 < data.slide_count)
            {
                for (int i=ticks.size()-2; i>=0; --i)
                {
                    double time = slide_end - ticks[i].time;
                    if (i!=0 || repeat+1!=data.slide_count-1)
                        on_event(make_hit_object( HitObjectType::slider_tick, i, time));
                    else
                    {
                        on_event(make_legacy_last_tick_object(slide_end - ticks[1].time));
                        on_event(make_hit_object( HitObjectType::slider_tail, i, time));
                    }
                }
            }
        }
    }

private:

    bool slider_velocity_affects_tick_distance;

    HitObject make_hit_object(HitObjectType type, size_t i, double time)
    {
        return HitObject{
            .type = type,
            .x = data.slider_head.x + ticks[i].position.X,
            .y = data.slider_head.y + ticks[i].position.Y,
            .time = data.slider_head.time + time,
        };

    }
    HitObject make_legacy_last_tick_object(double penultimateTickTime)
    {
        SliderTick legacy_last_tick = calculate_legacy_last_tick(penultimateTickTime);

        return HitObject{
            .type = HitObjectType::slider_legacy_last_tick,
            .x = data.slider_head.x + legacy_last_tick.position.X,
            .y = data.slider_head.y + legacy_last_tick.position.Y,
            .time = data.slider_head.time + legacy_last_tick.time,
        };
    }

    SliderTick calculate_legacy_last_tick(double penultimateTickTime)
    {
        constexpr double legacy_last_tick_offset = 36.0;
        double spanDuration = tick_duration * path_length / tick_distance;
;
        int finalSpanIndex = data.slide_count - 1;
        double finalSpanStartTime = finalSpanIndex * spanDuration;
        double totalDuration = data.slide_count * spanDuration;

        // adding penultimateTickTime to the calculation differs from lazer code, but matches difficulty calc instead.
        // The actual LegacyLastTick isn't even guaranteed to be the final tick, so it doesn't make sense to use it for difficulty calculation
        double legacyLastTickTime = std::max({penultimateTickTime, totalDuration / 2, (finalSpanStartTime + spanDuration) - legacy_last_tick_offset});
        double finalProgress = (legacyLastTickTime - finalSpanStartTime) / spanDuration;
        if (data.slide_count % 2 == 0) finalProgress = 1 - finalProgress;

        double distance = finalProgress*path_length;

        return {legacyLastTickTime, position(distance) };
    }

    void calculate_path()
    {
        auto begin = data.control_points.begin();
        auto end = data.control_points.end();
        auto next = begin;
        while(true)
        {
            auto current = next;
            ++next;
            if (next==end || next->new_slider_type != slider_type::None) // reached new segment
            {
                calculate_segment_path(path, {begin, next});
                if (next==end)
                    break;
                else
                    begin=next;
            }
        }
    }

    void calculate_distances()
    {
        double current_distance=0;
        cumulative_distance.resize(path.size());
        cumulative_distance[0] = 0;

        for (int i=1, last=path.size()-1; i<=last ; ++i)
        {
            Vector2 current_point = path[i-1];
            Vector2 next_point = path[i];
            current_distance += (next_point-current_point).length();

            cumulative_distance[i]= current_distance;
            if (current_distance > data.length)
            {
                path.resize(i+1);
                cumulative_distance.resize(i+1);
                break;
            }
        }

        if (data.control_points.size() >= 2 && data.control_points.back().position == (data.control_points.end()-2)->position && data.length > current_distance)
        {
            path_length = cumulative_distance.back();
            return;
        }
        path_length = data.length;
    }

    Vector2 position(size_t lower_bound, double distance)
    {
        double path_segment_length = (cumulative_distance[lower_bound+1] - cumulative_distance[lower_bound]);
        if (path_segment_length <1e-7)
        {
            return path[lower_bound];
        }

        return lerp(
            path[lower_bound],
            path[lower_bound+1],
            (distance-cumulative_distance[lower_bound]) / path_segment_length
            );
    }

    Vector2 position(double distance)
    {
        // assume approximately equidistant points, linear search to find exact location and lerp
        size_t approx_location = (distance / path_length) * cumulative_distance.size();

        if (approx_location < cumulative_distance.size())
        {
            if (cumulative_distance[approx_location]>distance)
            {
                for (size_t i=approx_location-1; i>=0 ; --i)
                {
                    if (cumulative_distance[i]<distance)
                        return position(i,distance);
                }
                return position(0,distance);
            }
            else {
                for (size_t i=approx_location+1, end=cumulative_distance.size(); i<end ; ++i)
                {
                    if (cumulative_distance[i]>distance)
                        return position(i-1,distance);
                }
            }
        }
        return position(cumulative_distance.size()-2,distance);
    }

    void calculate_ticks()
    {
        ticks.push_back({0, {0,0}});

        const double velocity = tick_distance/tick_duration;
        const double minDistanceFromEnd = velocity * 10;

        if (tick_distance != 0)
        {
            for (double d = tick_distance, t=tick_duration; d < path_length - minDistanceFromEnd; d += tick_distance, t+=tick_duration)
                ticks.push_back({t, position(d)});
        }
        ticks.push_back({path_length/velocity, position(cumulative_distance.size()-2,path_length)});

    }

    std::vector<Vector2> path;
    std::vector<double> cumulative_distance;
    std::vector<SliderTick> ticks;

    double tick_distance;
    double tick_duration;
    double path_length;
};

}
