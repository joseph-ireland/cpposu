// Copyright (c) ppy Pty Ltd <contact@ppy.sh>.
// Licensed under the MIT Licence - https://raw.githubusercontent.com/ppy/osu-framework/master/LICENCE

#pragma once

#include <cpposu/types.hpp>

namespace cpposu {


// code/comments based on OsuBeatmapProcessor.cs: private void applyStackingOld(Beatmap<OsuHitObject> beatmap)
inline std::vector<int> calculate_legacy_stack_heights(std::span<const HitObject> hitObjects, double time_threshold, float distance_threshold)
{
    std::vector<int> stackHeights(hitObjects.size());

    float d_squared = distance_threshold * distance_threshold;
    stackHeights.resize(hitObjects.size());

    int i=0;
    std::optional<Vector2> sliderPathEnd;

    while( i < hitObjects.size())
    {
        const HitObject& currHitObject = hitObjects[i];
        auto& stackHeight = stackHeights[i];
        ++i;
        while (i < hitObjects.size() && !is_start_event(hitObjects[i].type))
        {
            auto type = hitObjects[i].type;
            if (!sliderPathEnd && (type == slider_repeat || type == slider_tail))
                sliderPathEnd = hitObjects[i].position();
            ++i;
        }

        if (stackHeight != 0 && currHitObject.type != slider_head)
            continue;

        std::optional<double> lastStackTime;
        int sliderStack = 0;

        for (int j = i; j < hitObjects.size(); j++)
        {
            if (!is_start_event(hitObjects[j].type))
                continue;

            if (!lastStackTime)
            {
                lastStackTime = hitObjects[j-1].time;
            }

            if (hitObjects[j].time - *lastStackTime > time_threshold)
                break;

            if ((currHitObject.position()-hitObjects[j].position()).squared_length() < d_squared)
            {
                stackHeight++;
                lastStackTime.reset();
            }
            else if (sliderPathEnd && (*sliderPathEnd - hitObjects[j].position()).squared_length() < d_squared)
            {
                // Case for sliders - bump notes down and right, rather than up and left.
                sliderStack++;
                stackHeights[j] -= sliderStack;
                lastStackTime.reset();
            }
        }
        sliderPathEnd.reset();
    }

    return stackHeights;
}

// code/comments based on OsuBeatmapProcessor.cs: private void applyStacking(Beatmap<OsuHitObject> beatmap, int startIndex, int endIndex)
inline std::vector<int> calculate_stack_heights(std::span<const HitObject> hitObjects, double time_threshold, float distance_threshold)
{
    std::vector<int> stackHeights(hitObjects.size());

    float d_squared = distance_threshold * distance_threshold;

    // Reverse pass for stack calculation.
    for (int i = hitObjects.size()-1; i > 0; i--)
    {
        int n = i;
        /* We should check every note which has not yet got a stack.
            * Consider the case we have two interwound stacks and this will make sense.
            *
            * o <-1      o <-2
            *  o <-3      o <-4
            *
            * We first process starting from 4 and handle 2,
            * then we come backwards on the i loop iteration until we reach 3 and handle 1.
            * 2 and 1 will be ignored in the i loop because they already have a stack value.
            */

        const HitObject& objectI = hitObjects[i];
        if (stackHeights[i] != 0 || !is_target_circle(objectI.type)) continue;


        /* If this object is a hitcircle, then we enter this "special" case.
            * It either ends with a stack of hitcircles only, or a stack of hitcircles that are underneath a slider.
            * Any other case is handled by the "is Slider" code below this.
            */
        if (objectI.type == HitObjectType::circle)
        {
            Vector2 sliderEndPos{};
            Vector2 currentStackPos=objectI.position();
            double currentStackTime=objectI.time;

            int currentStackHeight = 0;
            while (--n >= 0)
            {
                if (currentStackTime - hitObjects[n].time > time_threshold)
                    // We are no longer within stacking range of the previous object.
                    break;

                if (hitObjects[n].type == slider_tail)
                    sliderEndPos = hitObjects[n].position();

                while (!is_start_event(hitObjects[n].type) && n>0)
                {

                    // skip to start event
                    --n;
                }
                auto& objectN = hitObjects[n];


                if (objectN.type == slider_head && (sliderEndPos - currentStackPos).squared_length() < d_squared)
                {
                    int offset = currentStackHeight - stackHeights[n] + 1;

                    for (int j = n + 1; j <= i; j++)
                    {
                        // For each object which was declared under this slider, we will offset it to appear *below* the slider end (rather than above).
                        if (is_target_circle(hitObjects[j].type) && (sliderEndPos - hitObjects[j].position()).squared_length() < d_squared)
                            stackHeights[j] -= offset;
                    }

                    // We have hit a slider.  We should restart calculation using this as the new base.
                    // Breaking here will mean that the slider still has StackCount of 0, so will be handled in the i-outer-loop.
                    break;
                }
                else if (is_target_circle(objectN.type))
                {
                    if ((objectN.position() - currentStackPos).squared_length() < d_squared)
                    {
                        // Keep processing as if there are no sliders.  If we come across a slider, this gets cancelled out.
                        //NOTE: Sliders with start positions stacking are a special case that is also handled here.

                        stackHeights[n] = ++currentStackHeight;
                        currentStackPos = objectN.position();
                        currentStackTime = objectN.time;
                    }
                }
            }
        }
        else if (objectI.type == HitObjectType::slider_head)
        {
            /* We have hit the first slider in a possible stack.
                * From this point on, we ALWAYS stack positive regardless.
                */

            int stackHeight=0;
            auto currentStackPosition=objectI.position();
            auto currentStackTime = objectI.time;
            while (--n >= 0)
            {
                Vector2 endPosition = hitObjects[n].position();
                while (!is_start_event(hitObjects[n].type) && n>0)
                {
                    // skip to start event
                    --n;
                }
                auto& objectN = hitObjects[n];

                if (currentStackTime - objectN.time > time_threshold)
                    // We are no longer within stacking range of the previous object.
                    break;

                if ((endPosition - currentStackPosition).squared_length() < d_squared)
                {
                    stackHeights[n] = ++stackHeight;
                    currentStackPosition=objectN.position();
                    currentStackTime = objectN.time;
                }
            }
        }
    }
    return stackHeights;
}

inline void apply_stacking(std::span<HitObject> hitObjects, int beatmapVersion, double timeThreshold, float distanceThreshold, float stackOffset)
{
    auto stackHeights = (beatmapVersion < 6)
        ? calculate_legacy_stack_heights(hitObjects, timeThreshold, distanceThreshold)
        : calculate_stack_heights(hitObjects, timeThreshold, distanceThreshold);

    int i=0;
    float totalOffset = 0;
    for (int i=0; i<hitObjects.size(); ++i)
    {
        if (is_start_event(hitObjects[i].type))
        {
            totalOffset = stackHeights[i] * stackOffset;
        }
        hitObjects[i].x += totalOffset;
        hitObjects[i].y += totalOffset;
    }
}

inline void apply_stacking(Beatmap& b)
{
    constexpr float distance_threshold=3;

    double time_preempt = (float)difficulty_range(b.difficulty_attributes.ApproachRate, 1800, 1200, 450);
    double time_threshold = time_preempt * b.info.StackLeniency;

    float scale = (1.0f - 0.7f * (b.difficulty_attributes.CircleSize - 5) / 5) / 2;
    float stackOffsetMult = scale * -6.4f;

    apply_stacking(b.hit_objects,b.version,time_threshold,distance_threshold,stackOffsetMult);
}
}
