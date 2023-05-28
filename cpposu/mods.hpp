#pragma once

#include "types.hpp"
#include "stacking.hpp"

namespace cpposu {


inline void apply_timescale(std::span<HitObject> hitObjects, double scale)
{
    for (auto& obj : hitObjects)
    {
        obj.time *= scale;
    }
}

inline void flip_horizontal(std::span<HitObject> hitObjects)
{
    for (auto& obj : hitObjects)
    {
        obj.x = 512 - obj.x;
    }

}

inline void flip_vertical(std::span<HitObject> hitObjects)
{
    for (auto& obj : hitObjects)
    {
        obj.y = 384 - obj.y;
    }
}
}
