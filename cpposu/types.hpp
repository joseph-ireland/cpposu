#pragma once

#include <algorithm>
#include <math.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <iostream>
#include <memory>
#include <span>
#include <array>

namespace cpposu {

enum HitObjectType : int
{
    circle,

    slider_head,
    slider_tick,
    slider_repeat,
    slider_legacy_last_tick,
    slider_tail,

    spinner_start,
    spinner_end,
};

inline bool is_start_event(HitObjectType t)
{
    switch(t)
    {
        case circle:
        case slider_head:
        case spinner_start:
            return true;
        default:
            return false;
    }
}
inline bool is_target_circle(HitObjectType t)
{
    switch(t)
    {
        case circle:
        case slider_head:
            return true;
        default:
            return false;
    }
}
struct Vector2 {
    float X,Y;

    Vector2 operator-() const { return {-X, -Y}; }
    Vector2 operator+(Vector2 b) const { return {X+b.X, Y+b.Y}; }
    Vector2 operator-(Vector2 b) const { return {X-b.X, Y-b.Y}; }
    Vector2 operator/(float a) const { return {X/a, Y/a}; }
    Vector2 operator*(float a) const  { return {X*a, Y*a}; }
    Vector2 operator+=(Vector2 a) { *this = *this + a; return *this;}
    Vector2 operator-=(Vector2 a) { *this = *this - a; return *this;}

    friend Vector2 operator*(float a, Vector2 b)  { return b*a; }
    friend Vector2 lerp(Vector2 a, Vector2 b, float t) { return (1-t)*a + t*b; }

    float squared_length() { return X*X+Y*Y; }
    float length() { return std::sqrt((double) squared_length()); }

    float dot(const Vector2& other) { return X*other.X + Y*other.Y; }

    auto operator<=>(const Vector2&) const = default;
};


constexpr const char* to_string(HitObjectType h)
{
    switch(h) {
    case HitObjectType::circle: return "circle";
    case HitObjectType::slider_head: return "slider_head";
    case HitObjectType::slider_tick: return "slider_tick";
    case HitObjectType::slider_repeat: return "slider_repeat";
    case HitObjectType::slider_legacy_last_tick: return "slider_legacy_last_tick";
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
    float HPDrainRate=5;
    float CircleSize=5;
    float OverallDifficulty=5;
    float ApproachRate=NAN;
    double SliderMultiplier=1;
    double SliderTickRate=1;
};
inline double difficulty_range(double difficulty, double min, double mid, double max)
{
    if (difficulty > 5)
        return mid + (max - mid) * (difficulty - 5) / 5;
    if (difficulty < 5)
        return mid - (mid - min) * (5 - difficulty) / 5;

    return mid;
}
struct BeatmapInfo
{
    std::string AudioFilename;
    double AudioLeadIn;
    double PreviewTime;
    std::string SampleSet;
    int SampleVolume;
    float StackLeniency=0.7;
    int Mode;
    bool LetterboxInBreaks;
    bool SpecialStyle;
    bool WidescreenStoryboard;
    bool EpilepsyWarning;
    bool SamplesMatchPlaybackRate;
    int Countdown;
    int CountdownOffset;
    std::string Title;
    std::string TitleUnicode;
    std::string Artist;
    std::string ArtistUnicode;
    std::string Creator;
    std::string Version;
    std::string Source;
    std::string Tags;
    uint64_t BeatmapID;
    uint64_t BeatmapSetID;

};

struct TimingPoint
{
    double time{};
    double beatLength{};
    int meter{};
    int sampleSet{};
    int sampleIndex{};
    int volume{};
    bool timing_change{};
    uint64_t effects{};
};

struct TimingPoints
{
    static constexpr double default_beat_length = 60000.0 / 60.0;
    std::vector<TimingPoint> points;
    double currentTime=-INFINITY;
    size_t nextIndex=0;
    double currentBeatLength=default_beat_length;
    double currentSliderVelocityMultiplier=1;
    double baseSliderVelocity=1;
    double sliderTickRate=1;

    double tickDistance(int beatmap_version)
    {
        if (beatmap_version >= 8)
            return  100 * currentSliderVelocityMultiplier * baseSliderVelocity / sliderTickRate;
        return 100 * baseSliderVelocity  / sliderTickRate;
    }
    double tickDuration(int beatmap_version)
    {
        if (beatmap_version >= 8)
            return currentBeatLength / sliderTickRate;
        return currentBeatLength / (sliderTickRate * currentSliderVelocityMultiplier);
    }
    void applyDefaults()
    {
        if (points.size()>0) currentBeatLength = points[0].beatLength;
    }
    void advanceTime(double time)
    {
        if (currentTime > time)
        {
            throw std::runtime_error("Time points accessed non-sequentially, probably an aspire map");
        }
        currentTime=time;
        while(nextIndex < points.size() && points[nextIndex].time <= time)
        {
            double currentTime = points[nextIndex].time;
            currentSliderVelocityMultiplier=1;

            do
            {
                size_t currentIndex = nextIndex;
                nextIndex++;
                if (points[currentIndex].timing_change)
                {
                    currentBeatLength = std::clamp(points[currentIndex].beatLength, 6.0, 60000.0);
                }
                else if (points[currentIndex].beatLength < 0)
                {
                    currentSliderVelocityMultiplier = std::clamp(-100/points[currentIndex].beatLength, 0.1, 10.0);
                }
            } while(nextIndex < points.size() && points[nextIndex].time == currentTime);
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

    float x, y;
    double time;
    auto operator <=>(const HitObject&) const = default;
    Vector2 position() const { return Vector2{x,y}; }
};

inline std::ostream& operator <<(std::ostream& os, const HitObject& h)
{
    return os << "HitObject( " << h.type << " x=" << h.x << " y=" << h.y << " time=" << h.time << ")";
}

struct Beatmap
{
    static constexpr int FIRST_LAZER_VERSION = 128;
    int version;

    BeatmapInfo info{};
    MapDifficultyAttributes difficulty_attributes{};
    TimingPoints timing_points;
    std::vector<HitObject> hit_objects;
};

enum class slider_type
{
    None=0,
    Bezier='B',
    CentripetalCatmullRom='C',
    Linear='L',
    PerfectCircle='P',
};

struct SliderControlPoint
{
    slider_type new_slider_type = slider_type::None;
    Vector2 position;
};



template <typename T, size_t N=512>
class Arena
{
    Arena(const Arena&) = delete;

    void grow(size_t required_size)
    {
        size = std::max(2*size, required_size);
        allocations.emplace_back(std::make_unique<T[]>(size));
        available = {allocations.back().get(), size};
    }

    std::array<T, N> stack_data;
    std::span<T> available = stack_data;
    size_t size = N;
    std::vector<std::unique_ptr<T[]>> allocations;

public:
    Arena() = default;

    std::span<T> take(size_t n)
    {
        if (n>=available.size()) [[unlikely]]
        {
            grow(n);
        }

        std::span<T> result = available.subspan(0,n);
        available = available.subspan(n, available.size()-n);
        return result;
    }
};


}
