#pragma once

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
    slider_legacy_last_tick,
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
    double HPDrainRate;
    double CircleSize;
    double OverallDifficulty;
    double ApproachRate;
    double SliderMultiplier;
    double SliderTickRate;
};

struct TimingPoint
{
    int64_t time{};
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
    double currentSliderVelocityMultiplier=1;
    double baseSliderVelocity=1;
    double sliderTickRate=1;

    double tickDistance() { return 100 * currentSliderVelocityMultiplier * baseSliderVelocity / sliderTickRate; }
    double tickDuration() { return currentBeatLength / sliderTickRate; }
    void advanceTime(int64_t time)
    {
        while(nextIndex < points.size() && points[nextIndex].time <= time)
        {
            size_t currentIndex = nextIndex;
            nextIndex++;
            if (points[currentIndex].uninherited)
            {
                currentBeatLength = points[currentIndex].beatLength;
                currentSliderVelocityMultiplier = 1;
            }
            else
            {
                currentSliderVelocityMultiplier = -100/points[currentIndex].beatLength;
            }
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

struct Vector2 {
    float X,Y;

    Vector2 operator-() const { return {-X, -Y}; }
    Vector2 operator+(Vector2 b) const { return {X+b.X, Y+b.Y}; }
    Vector2 operator-(Vector2 b) const { return {X-b.X, Y-b.Y}; }
    Vector2 operator/(float a) const { return {X/a, Y/a}; }
    Vector2 operator*(float a) const  { return {a*X, a*Y}; }
    friend Vector2 operator*(float a, Vector2 b)  { return b*a; }
    friend Vector2 lerp(Vector2 a, Vector2 b, float t) { return (1-t)*a + t*b; }

    float squared_length() { return X*X+Y*Y; }
    float length() { return std::sqrtf(squared_length()); }

    float dot(const Vector2& other) { return X*other.X + Y*other.Y; }

    auto operator<=>(const Vector2&) const = default;
};



template <typename T, size_t N=2048>
class Arena
{
    Arena(const Arena&) = delete;
    struct segment {
        std::array<T,N> data;
        size_t index=0;
    };
    void grow()
    {
        segments.emplace_back();
        current = &segments.back();
    }
    segment first;
    segment* current = &first;
    std::vector<segment> segments;
    std::vector<std::unique_ptr<T[]>> large_allocations;

public:
    Arena() = default;

    std::span<T> take(size_t n)
    {
        if (current->index+n>=N) [[unlikely]]
        {
            if (3*n >= N)
            {
                auto data = std::make_unique<T[]>(n);
                std::span<T> result {data.get(), n};
                large_allocations.emplace_back(std::move(data));
                return result;
            }
            grow();
        }
        std::span<T> result{&current->data[current->index], n};
        current->index += n;
        return result;
    }
};


}
