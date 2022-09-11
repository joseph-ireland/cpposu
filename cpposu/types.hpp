#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <iostream>

namespace cpposu {

enum HitObjectType : int
{
    circle,
    
    slider_head,
    slider_tick,
    slider_tail,

    spinner_start,
    spinner_end,
};

constexpr const char* to_string(HitObjectType h)
{
    switch(h) {
    case HitObjectType::circle: return "circle";
    case HitObjectType::slider_head: return "slider_head";
    case HitObjectType::slider_tick: return "slider_tick";
    case HitObjectType::slider_tail: return "slider_tail";
    case HitObjectType::spinner_start: return "spinner_start";
    case HitObjectType::spinner_end: return "spinner_end";
    }
    return "unknown";
}

inline std::ostream& operator<<(std::ostream& os, HitObjectType h)
{
    return os << to_string(h);
}

struct MapDifficultyAttributes
{
    double HPDrainRate;
    double CircleSize;
    double OverallDifficulty;
    double ApproachRate;
    double SliderMultiplier;
    double SliderTickRate;
};

struct TimingPoint
{
    uint64_t time{};
    double beatLength{};
    int meter{};
    int sampleSet{};
    int sampleIndex{};
    int volume{};
    bool uninherited{};
    uint64_t effects{};
};

struct TimingPoints
{
    std::vector<TimingPoint> points;
    size_t nextIndex=0;
    double currentBeatLength=0;
    double currentSliderVelocity=1;
    void advanceTime(uint64_t time)
    {
        while(nextIndex < points.size() && points[nextIndex].time < time)
        {
            size_t currentIndex = nextIndex; 
            nextIndex++;
            if (points[currentIndex].uninherited)
                currentBeatLength = points[currentIndex].beatLength;
            else
                currentSliderVelocity = -0.01 * points[currentIndex].beatLength;
        }
    }
};

enum HitObjectTypeFlags
{
    HitCircleFlag=1<<0,
    SliderFlag=1<<1,
    SpinnerFlag=1<<3,
};

struct HitObject
{
    HitObjectType type;
        
    int x, y, time;
    auto operator <=>(const HitObject&) const = default;
};

inline std::ostream& operator <<(std::ostream& os, const HitObject& h)
{
    return os << "HitObject( " << h.type << " x=" << h.x << " y=" << h.y << " time=" << h.time << ")";
}

struct Beatmap
{
    int version;

    std::optional<std::unordered_map<std::string, std::string>> general, metadata; 
    MapDifficultyAttributes difficulty_attributes;
    TimingPoints timing_points;
    std::vector<HitObject> hit_objects;
};

}