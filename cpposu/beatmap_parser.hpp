#pragma once

#include <cpposu/slider.hpp>
#include <cpposu/line_parser.hpp>
#include <cpposu/types.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <sstream>
#include <charconv>


namespace cpposu
{



struct BeatmapParser : LineParser
{
    using LineParser::LineParser;

    Beatmap parse();


protected:

    static bool is_section_start(std::string_view line)
    {
        return line.starts_with('[');
    }


    void parse_header();
    void parse_section();

    template <typename Func>
    void parse_section(std::string_view first_line, Func&& f)
    {
        if (!first_line.empty()) f(first_line); // handle lack of new line after section header, which happens occasionally
        std::string_view line;
        while(line=read_line(), !check_section_complete(line))
            f(line);
    }

    std::unordered_map<std::string, std::string> parse_dict_section(std::string_view first_line)
    {
        std::unordered_map<std::string, std::string> result;
        parse_section(first_line, [&](auto line){
            auto key = take_column(line, ':');
            auto val = trim_space(line);
            result.emplace(key,val);
        });

        return result;
    }

    void parse_hit_objects(std::string_view first_line)
    {
        parse_section(first_line, [&](auto line){
            HitObject h;
            take_numeric_column(h.x, line);
            h.x = std::truncf(h.x);
            take_numeric_column(h.y, line);
            h.y = std::truncf(h.y);

            take_numeric_column(h.time, line);
            if (!beatmap_.hit_objects.empty() &&  beatmap_.hit_objects.back().time - h.time > 1000)
            {
                CPPOSU_RAISE_PARSE_ERROR("Likely unsupported aspire map - went back in time by "
                    << (beatmap_.hit_objects.back().time - h.time) << " ms."
                    << " Hit object at time " << h.time << " appears later than " << beatmap_.hit_objects.back());
            }
            uint32_t type;
            take_numeric_column(type, line);
            try_take_column(line); // hit sound -- unused
            if (type & SpinnerFlag)
                parse_spinner(h, line);
            else if (type & SliderFlag)
                parse_slider(h, line);
            else if (type & HitCircleFlag) {
                h.type = HitObjectType::circle;
                beatmap_.hit_objects.push_back(h);
            }
        });
    }

    void parse_spinner(HitObject spinner_start, std::string_view extras)
    {
        spinner_start.x = 256;
        spinner_start.y = 192;
        HitObject spinner_end = spinner_start;
        spinner_start.type = HitObjectType::spinner_start;
        spinner_end.type = HitObjectType::spinner_end;
        take_numeric_column(spinner_end.time, extras);
        spinner_end.time = std::max(spinner_end.time, spinner_start.time);
        beatmap_.hit_objects.push_back(spinner_start);
        beatmap_.hit_objects.push_back(spinner_end);
    }

    slider_type parse_slider_type(std::string_view s)
    {
        auto result = try_parse_slider_type(s);
        if (!result) CPPOSU_RAISE_PARSE_ERROR("invalid slider type: " << s);
        return *result;
    }
    Vector2 parse_slider_position(std::string_view s)
    {
        Vector2 result;
        take_numeric_column(result.X, s, ':');
        read_number_or_throw(result.Y, s);
        return result;
    }

    void parse_slider(HitObject slider_head, std::string_view extras)
    {
        slider_head.type = HitObjectType::slider_head;
        slider_.data.slider_head = slider_head;

        std::string_view path_data = take_column(extras);

        slider_.data.slide_count = std::max(take_numeric_column<int>(extras), 1);
        slider_.data.length = take_numeric_column<double>(extras);

        slider_type initial_slider_type = parse_slider_type(take_column(path_data,'|'));

        slider_type current_slider_type = initial_slider_type;
        size_t current_segment_start=0;
        auto validate_segment = [&](){
            if (current_slider_type == slider_type::PerfectCircle)
            {
                std::span<SliderControlPoint> segment(
                    slider_.data.control_points.begin()+current_segment_start,
                    slider_.data.control_points.end());

                if (segment.size() == 0) return;

                if(segment.size() != 3)
                {
                    segment.front().new_slider_type = slider_type::Bezier;
                    return;
                }
                auto& p = segment;
                const bool isLinear = std::abs((p[1].position.Y - p[0].position.Y) * (p[2].position.X - p[0].position.X)
                                    - (p[1].position.X - p[0].position.X) * (p[2].position.Y - p[0].position.Y)) < 1e-3;
                if (isLinear)
                {
                    segment.front().new_slider_type = slider_type::Linear;
                }
            }
        };

        slider_.data.control_points.clear();
        slider_.data.control_points.push_back({current_slider_type, {slider_head.x, slider_head.y}});
        while(std::optional<std::string_view> control_point_str = try_take_column(path_data, '|'))
        {
            if (control_point_str->size() == 1)
            {
                SliderControlPoint p{
                    parse_slider_type(*control_point_str),
                    parse_slider_position(take_column(path_data, '|'))
                };

                // push end of current slider segment, plus start of new segment
                // note: different to lazer implementation - all slider segments must contain an end point.
                // This allows us to more easily do the rest of the parsing in one pass,
                // whereas lazer takes several.
                slider_.data.control_points.push_back({slider_type::None, p.position});

                validate_segment();

                current_slider_type = p.new_slider_type;
                current_segment_start = slider_.data.control_points.size();
                slider_.data.control_points.push_back(p);
            }
            else
            {
                slider_.data.control_points.push_back({
                    slider_type::None,
                    parse_slider_position(*control_point_str)
                });
            }
        }
        validate_segment();

        if (!slider_.data.control_points.empty()) current_slider_type = slider_.data.control_points.front().new_slider_type;

        for (size_t i=1; i<slider_.data.control_points.size()-1; ++i)
        {

            auto& prev=slider_.data.control_points[i-1];
            auto& current = slider_.data.control_points[i];
            auto& next = slider_.data.control_points[i+1];

            // The last control point of each segment is not allowed to start a new implicit segment.
            if (next.new_slider_type != slider_type::None)
            {
                current_slider_type = next.new_slider_type;
                continue;
            }

            // Keep incrementing while an implicit segment doesn't need to be started.
            if (prev.position != current.position)
                continue;

            // Legacy Catmull sliders don't support multiple segments, so adjacent Catmull segments should be treated as a single one.
            // Importantly, this is not applied to the first control point, which may duplicate the slider path's position
            // resulting in a duplicate (0,0) control point in the resultant list.
            if (current_slider_type == slider_type::CentripetalCatmullRom
                    && i > 1 && beatmap_.version < Beatmap::FIRST_LAZER_VERSION)
                continue;

            // create new implicit slider segment
            current.new_slider_type = current_slider_type;
        }

        // control points are calculated relative to slider head
        for (auto& p : slider_.data.control_points)
        {
            p.position -= Vector2{slider_head.x, slider_head.y};
        }


        slider_.generate_hit_objects(beatmap_.timing_points, beatmap_.version, [this](auto&& ho){
            beatmap_.hit_objects.push_back(ho);
        });
    }

    void parse_metadata(std::string_view first_line);
    void parse_difficulty(std::string_view first_line)
    {
        #define CPPOSU_DIFFICULTY_VAR(var) if (key == #var) beatmap_.difficulty_attributes.var = val;
        parse_section(first_line, [&](auto line){
            auto key = take_column(line, ':');
            auto val = read_number_or_throw<double>(trim_space(line));

            CPPOSU_DIFFICULTY_VAR(HPDrainRate);
            CPPOSU_DIFFICULTY_VAR(CircleSize);
            CPPOSU_DIFFICULTY_VAR(OverallDifficulty);
            CPPOSU_DIFFICULTY_VAR(ApproachRate);
            CPPOSU_DIFFICULTY_VAR(SliderMultiplier);
            CPPOSU_DIFFICULTY_VAR(SliderTickRate);
        });
        #undef CPPOSU_DIFFICULTY_VAR

        beatmap_.timing_points.baseSliderVelocity = beatmap_.difficulty_attributes.SliderMultiplier;
        beatmap_.timing_points.sliderTickRate = beatmap_.difficulty_attributes.SliderTickRate;

    }

    void parse_timing_points(std::string_view first_line)
    {
        parse_section(first_line, [&](auto line){
            TimingPoint t;
            take_numeric_column(t.time, line);
            take_numeric_column(t.beatLength, line);
            if(try_take_numeric_column(t.meter, line))
            {
                if (t.meter <= 0)
                    return; // lazer throws exception and aborts processing
            }
            std::ignore = try_take_numeric_column(t.sampleSet, line);
            std::ignore = try_take_numeric_column(t.sampleIndex, line);
            std::ignore = try_take_numeric_column(t.volume, line);
            if (!try_take_numeric_column(t.timing_change, line))
            {
                t.timing_change = t.beatLength >= 0;
            }
            std::ignore = try_take_numeric_column(t.effects, line);
            beatmap_.timing_points.points.push_back(t);
        });
        beatmap_.timing_points.applyDefaults();
    }

    void ignore_section()
    {
        while(!check_section_complete(read_line())) {}
    }


    bool check_section_complete(std::string_view line)
    {
        return line.empty() || is_section_start(line);
    }

    Beatmap beatmap_;
    Slider slider_;

};



inline Beatmap BeatmapParser::parse()
{
    parse_header();
    read_line(); // parse_section consumes a line that has already been read (needed to detect section begin)

    while(stream_) parse_section();

    return beatmap_;
}

inline void BeatmapParser::parse_header()
{
    auto line = read_line();

    std::string_view bom = "\xEF\xBB\xBF";
    try_take_prefix(line, bom);
    if(line.empty()) line = read_line();

    std::string_view prefix_string = "osu file format v";

    if (!try_take_prefix(line, prefix_string))
        CPPOSU_RAISE_PARSE_ERROR("Invalid file prefix, expected \"" << prefix_string << debug_location(line));

    beatmap_.version = read_number_or_throw<int>(line);
}

inline void BeatmapParser::parse_section()
{
    auto line = reread_last_line();
    if (!is_section_start(line))
        CPPOSU_RAISE_PARSE_ERROR("Expected section start: " << debug_location(line));

    if(try_take_prefix(line, "[General]"))
        beatmap_.general = parse_dict_section(line);
    else if (try_take_prefix(line, "[Metadata]"))
        beatmap_.metadata = parse_dict_section(line);
    else if (try_take_prefix(line, "[Difficulty]"))
        parse_difficulty(line);
    else if (try_take_prefix(line, "[TimingPoints]"))
        parse_timing_points(line);
    else if (try_take_prefix(line, "[HitObjects]"))
        parse_hit_objects(line);
    else
        ignore_section();
}

}
