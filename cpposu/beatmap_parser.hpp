#pragma once

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
            auto val = line;
            result.emplace(key,val);
        });

        return result;
    }

    void parse_hit_objects(std::string_view first_line)
    {
        parse_section(first_line, [&](auto line){
            HitObject h;
            take_numeric_column(h.x, line);
            take_numeric_column(h.y, line);
            take_numeric_column(h.time, line);
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

    void parse_spinner(HitObject spinner_start, std::string_view line)
    {
        HitObject spinner_end = spinner_start;
        spinner_start.type = HitObjectType::spinner_start;
        spinner_end.type = HitObjectType::spinner_end;
        take_numeric_column(spinner_end.time, line);
        beatmap_.hit_objects.push_back(spinner_start);
        beatmap_.hit_objects.push_back(spinner_end);
    }

    void parse_slider(HitObject slider_head, std::string_view line)
    {
        // TODO sliders
        slider_head.type = HitObjectType::slider_head;
        beatmap_.hit_objects.push_back(slider_head);
    }

    void parse_metadata(std::string_view first_line);
    void parse_difficulty(std::string_view first_line)
    {
        #define CPPOSU_DIFFICULTY_VAR(var) if (key == #var) beatmap_.difficulty_attributes.var = val;
        parse_section(first_line, [&](auto line){
            auto key = take_column(line, ':');
            auto val = take_number<double>(line);

            CPPOSU_DIFFICULTY_VAR(HPDrainRate);
            CPPOSU_DIFFICULTY_VAR(CircleSize);
            CPPOSU_DIFFICULTY_VAR(OverallDifficulty);
            CPPOSU_DIFFICULTY_VAR(ApproachRate);
            CPPOSU_DIFFICULTY_VAR(SliderMultiplier);
            CPPOSU_DIFFICULTY_VAR(SliderTickRate);
        });
        #undef CPPOSU_DIFFICULTY_VAR
    }

    void parse_timing_points(std::string_view first_line)
    {
        parse_section(first_line, [&](auto line){
            TimingPoint t;
            take_numeric_column(t.time, line);
            take_numeric_column(t.beatLength, line);
            take_numeric_column(t.meter, line);
            take_numeric_column(t.sampleSet, line);
            take_numeric_column(t.sampleIndex, line);
            take_numeric_column(t.volume, line);
            take_numeric_column(t.uninherited, line);
            try_take_numeric_column(t.effects, line);
            beatmap_.timing_points.points.push_back(t);
        });
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
    std::string_view prefix_string = "osu file format v";

    if (!try_take_prefix(line, prefix_string))
        CPPOSU_RAISE_PARSE_ERROR("Invalid file prefix, expected \"" << prefix_string << debug_location(line));

    beatmap_.version = take_number<int>(line);
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