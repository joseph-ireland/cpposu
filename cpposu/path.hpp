#pragma once

// Copyright (c) 2007-2018 ppy Pty Ltd <contact@ppy.sh>.
// Licensed under the MIT Licence - https://raw.githubusercontent.com/ppy/osu-framework/master/LICENCE

#include <cmath>
#include <numbers>
#include <cpposu/types.hpp>

#include <span>
#include <vector>
#include <array>
#include <stdexcept>
#include <algorithm>


namespace cpposu
{

    namespace detail
    {
        inline constexpr float bezier_tolerance = 0.25f;

        /// <summary>
        /// The amount of pieces to calculate for each control point quadruplet.
        /// </summary>
        inline constexpr int catmull_detail = 50;

        inline constexpr float circular_arc_tolerance = 0.1f;
        /// <summary>
        /// Make sure the 2nd order derivative (approximated using finite elements) is within tolerable bounds.
        /// NOTE: The 2nd order derivative of a 2d curve represents its curvature, so intuitively this function
        ///       checks (as the name suggests) whether our approximation is _locally_ "flat". More curvy parts
        ///       need to have a denser approximation to be more "flat".
        /// </summary>
        /// <param name="controlPoints">The control points to check for flatness.</param>
        /// <returns>Whether the control points are flat enough.</returns>
        inline bool bezierIsFlatEnough(std::span<Vector2> controlPoints)
        {
            for (int i = 1; i < controlPoints.size() - 1; i++)
            {
                if ((controlPoints[i - 1] - 2 * controlPoints[i] + controlPoints[i + 1]).squared_length() > bezier_tolerance * bezier_tolerance * 4)
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Subdivides n control points representing a bezier curve into 2 sets of n control points, each
        /// describing a bezier curve equivalent to a half of the original curve. Effectively this splits
        /// the original curve into 2 curves which result in the original curve when pieced back together.
        /// </summary>
        /// <param name="controlPoints">The control points to split.</param>
        /// <param name="l">Output: The control points corresponding to the left half of the curve.</param>
        /// <param name="r">Output: The control points corresponding to the right half of the curve.</param>
        /// <param name="subdivisionBuffer">The first buffer containing the current subdivision state.</param>
        /// <param name="count">The number of control points in the original list.</param>
        inline void bezierSubdivide(std::span<Vector2> controlPoints, std::span<Vector2> l, std::span<Vector2> r, std::span<Vector2> subdivisionBuffer, int count)
        {
            std::span midpoints = subdivisionBuffer;

            for (int i = 0; i < count; ++i)
                midpoints[i] = controlPoints[i];

            for (int i = 0; i < count; i++)
            {
                l[i] = midpoints[0];
                r[count - i - 1] = midpoints[count - i - 1];

                for (int j = 0; j < count - i - 1; j++)
                    midpoints[j] = (midpoints[j] + midpoints[j + 1]) * 0.5;
            }
        }

        /// <summary>
        /// This uses <a href="https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm">De Casteljau's algorithm</a> to obtain an optimal
        /// piecewise-linear approximation of the bezier curve with the same amount of points as there are control points.
        /// </summary>
        /// <param name="controlPoints">The control points describing the bezier curve to be approximated.</param>
        /// <param name="output">The points representing the resulting piecewise-linear approximation.</param>
        /// <param name="count">The number of control points in the original list.</param>
        /// <param name="subdivisionBuffer1">The first buffer containing the current subdivision state.</param>
        /// <param name="subdivisionBuffer2">The second buffer containing the current subdivision state.</param>
        inline void bezierApproximate(std::span<Vector2> controlPoints, std::vector<Vector2>& output, std::span<Vector2> subdivisionBuffer1, std::span<Vector2> subdivisionBuffer2, int count)
        {
            std::span<Vector2> l = subdivisionBuffer2;
            std::span<Vector2> r = subdivisionBuffer1;

            bezierSubdivide(controlPoints, l, r, subdivisionBuffer1, count);

            for (int i = 0; i < count - 1; ++i)
                l[count + i] = r[i + 1];

            output.push_back(controlPoints[0]);

            for (int i = 1; i < count - 1; ++i)
            {
                int index = 2 * i;
                Vector2 p = 0.25f * (l[index - 1] + 2 * l[index] + l[index + 1]);
                output.push_back(p);
            }
        }

        /// <summary>
        /// Finds a point on the spline at the position of a parameter.
        /// </summary>
        /// <param name="vec1">The first vector.</param>
        /// <param name="vec2">The second vector.</param>
        /// <param name="vec3">The third vector.</param>
        /// <param name="vec4">The fourth vector.</param>
        /// <param name="t">The parameter at which to find the point on the spline, in the range [0, 1].</param>
        /// <returns>The point on the spline at <paramref name="t"/>.</returns>
        inline Vector2 catmullFindPoint(Vector2 vec1, Vector2 vec2, Vector2 vec3, Vector2 vec4, float t)
        {
            float t2 = t * t;
            float t3 = t * t2;

            Vector2 result;
            result.X = 0.5f * (2 * vec2.X + (-vec1.X + vec3.X) * t + (2 * vec1.X - 5 * vec2.X + 4 * vec3.X - vec4.X) * t2 + (-vec1.X + 3 * vec2.X - 3 * vec3.X + vec4.X) * t3);
            result.Y = 0.5f * (2 * vec2.Y + (-vec1.Y + vec3.Y) * t + (2 * vec1.Y - 5 * vec2.Y + 4 * vec3.Y - vec4.Y) * t2 + (-vec1.Y + 3 * vec2.Y - 3 * vec3.Y + vec4.Y) * t3);

            return result;
        }
    }
    void ApproximateBSpline(std::vector<Vector2>& output, std::span<Vector2> controlPoints, int p = 0);


    /// <summary>
    /// Creates a piecewise-linear approximation of a bezier curve, by adaptively repeatedly subdividing
    /// the control points until their approximation error vanishes below a given threshold.
    /// </summary>
    /// <param name="controlPoints">The control points.</param>
    /// <returns>A list of vectors representing the piecewise-linear approximation.</returns>
    inline void ApproximateBezier(std::vector<Vector2>& output, std::span<const SliderControlPoint> controlPoints)
    {
        int p = controlPoints.size() - 1;

        if (p < 0)
            return;

        Arena<Vector2> arena;

        auto Pop = [](auto& vec) { auto result = vec.back(); vec.pop_back(); return result; };

        std::vector<std::span<Vector2>> toFlatten;
        std::vector<std::span<Vector2>> freeBuffers;

        auto inputPoints = arena.take(controlPoints.size());
        {
            size_t i =0;
            for (const auto& point : controlPoints)
            {
                inputPoints[i++] = point.position;
            }
        }

        toFlatten.push_back(inputPoints);
        // "toFlatten" contains all the curves which are not yet approximated well enough.
        // We use a stack to emulate recursion without the risk of running into a stack overflow.
        // (More specifically, we iteratively and adaptively refine our curve with a
        // <a href="https://en.wikipedia.org/wiki/Depth-first_search">Depth-first search</a>
        // over the tree resulting from the subdivisions we make.)

        std::span<Vector2> subdivisionBuffer1 = arena.take(p+1);
        std::span<Vector2> subdivisionBuffer2 = arena.take(p * 2 + 1);

        std::span<Vector2> leftChild = subdivisionBuffer2;

        while (toFlatten.size() > 0)
        {
            std::span<Vector2> parent = Pop(toFlatten);

            if (detail::bezierIsFlatEnough(parent))
            {
                // If the control points we currently operate on are sufficiently "flat", we use
                // an extension to De Casteljau's algorithm to obtain a piecewise-linear approximation
                // of the bezier curve represented by our control points, consisting of the same amount
                // of points as there are control points.
                detail::bezierApproximate(parent, output, subdivisionBuffer1, subdivisionBuffer2, p + 1);

                freeBuffers.push_back(parent);
                continue;
            }

            // If we do not yet have a sufficiently "flat" (in other words, detailed) approximation we keep
            // subdividing the curve we are currently operating on.
            std::span<Vector2> rightChild = freeBuffers.size() > 0 ? Pop(freeBuffers) : arena.take(p+1);
            detail::bezierSubdivide(parent, leftChild, rightChild, subdivisionBuffer1, p + 1);

            // We re-use the buffer of the parent for one of the children, so that we save one allocation per iteration.
            for (int i = 0; i < p + 1; ++i)
                parent[i] = leftChild[i];

            toFlatten.push_back(rightChild);
            toFlatten.push_back(parent);
        }

        output.push_back(controlPoints[p].position);
    }

    /// <summary>
    /// Creates a piecewise-linear approximation of a Catmull-Rom spline.
    /// </summary>
    /// <returns>A list of vectors representing the piecewise-linear approximation.</returns>
    inline void ApproximateCatmull(std::vector<Vector2>& out, std::span<const SliderControlPoint> controlPoints)
    {
        out.reserve(out.size()+(controlPoints.size() - 1) * detail::catmull_detail * 2);
        for (int i = 0; i < controlPoints.size() - 1; i++)
        {
            auto v1 = i > 0 ? controlPoints[i - 1].position : controlPoints[i].position;
            auto v2 = controlPoints[i].position;
            auto v3 = i < controlPoints.size() - 1 ? controlPoints[i + 1].position : v2 + v2 - v1;
            auto v4 = i < controlPoints.size() - 2 ? controlPoints[i + 2].position : v3 + v3 - v2;

            for (int c = 0; c < detail::catmull_detail; c++)
            {
                out.push_back(detail::catmullFindPoint(v1, v2, v3, v4, (float)c / detail::catmull_detail));
                out.push_back(detail::catmullFindPoint(v1, v2, v3, v4, (float)(c + 1) / detail::catmull_detail));
            }
        }
    }

    struct CircularArc
    {
        Vector2 Centre;
        int AmountPoints;
        float Radius;
        double ThetaStart, ThetaRange, Direction, LengthToAngleMultiplier;

        Vector2 position_at_distance(float distance)
        {
            float theta = distance * LengthToAngleMultiplier;
            float theta_inc = Direction*ThetaRange/(AmountPoints-1);
            float theta_0 = theta_inc*std::min(std::floor(theta/theta_inc), AmountPoints-2.0f);
            float theta_1 = theta_0+theta_inc;
            float t = (theta-theta_0)/theta_inc;

            theta_0 += ThetaStart;
            theta_1 += ThetaStart;

            auto point_0 = position_at_theta(theta_0);
            auto point_1 = position_at_theta(theta_1);
            return lerp(point_0, point_1, t);
        }

        Vector2 position_at_theta(double theta)
        {
            return Centre + Radius * Vector2{(float)std::cos(theta), (float)std::sin(theta)};
        }

        Vector2 position_at_distance_nolerp(float distance)
        {
            float theta = ThetaStart + distance * LengthToAngleMultiplier;
            return position_at_theta(distance);
        }

        void Approximate(std::vector<Vector2>& out)
        {
            out.reserve(out.size()+AmountPoints);

            for(int i=0; i<AmountPoints; ++i)
            {
                double fract = i / double(AmountPoints - 1);
                double theta = ThetaStart + Direction * fract * ThetaRange;
                out.push_back(position_at_theta(theta));
            }
        }


        static std::optional<CircularArc> fromControlPoints(std::span<const SliderControlPoint> controlPoints)
        {
            if (controlPoints.size() != 3) return {};

            Vector2 a = controlPoints[0].position;
            Vector2 b = controlPoints[1].position;
            Vector2 c = controlPoints[2].position;

            // If we have a degenerate triangle where a side-length is almost zero, then give up and fallback to a more numerically stable method.
            if (std::abs((b.Y - a.Y) * (c.X - a.X) - (b.X - a.X) * (c.Y - a.Y)) < 1e-3f)
            {
                return {};
            }

            // See: https://en.wikipedia.org/wiki/Circumscribed_circle#Cartesian_coordinates_2
            float d = 2 * (a.X * (b - c).Y + b.X * (c - a).Y + c.X * (a - b).Y);
            float aSq = a.squared_length();
            float bSq = b.squared_length();
            float cSq = c.squared_length();

            CircularArc result;

            result.Centre = Vector2{
                aSq * (b - c).Y + bSq * (c - a).Y + cSq * (a - b).Y,
                aSq * (c - b).X + bSq * (a - c).X + cSq * (b - a).X} / d;

            Vector2 dA = a - result.Centre;
            Vector2 dC = c - result.Centre;

            result.Radius = dA.length();

            result.ThetaStart = std::atan2((double)dA.Y, (double)dA.X);

            result.Direction = 1;
            double thetaEnd = std::atan2((double)dC.Y, (double)dC.X);

            while (thetaEnd < result.ThetaStart)
                thetaEnd += 2 * std::numbers::pi;

            result.ThetaRange = thetaEnd - result.ThetaStart;


            // Decide in which direction to draw the circle, depending on which side of
            // AC B lies.
            Vector2 vecAC = c - a;
            Vector2 orthoAC {vecAC.Y, -vecAC.X};

            if (orthoAC.dot(b - a) < 0)
            {
                result.Direction = -1;
                result.ThetaRange = 2 * std::numbers::pi - result.ThetaRange;
            }

            result.LengthToAngleMultiplier = result.Direction / result.Radius;

            // osu! approximates circles as linear segments with below tolerance
            // This makes the overall path shorter
            constexpr float circular_arc_tolerance = 0.1;
            if (circular_arc_tolerance < 2*result.Radius)
            {
                double point_count = result.ThetaRange / (2*std::acos((double)(1.0f - circular_arc_tolerance / result.Radius)));
                result.AmountPoints = std::max(2.0,std::ceil(point_count));
            }
            else  {
                result.AmountPoints = 2;
            }
            double alpha = result.ThetaRange / (2*(result.AmountPoints-1));

            result.LengthToAngleMultiplier *= alpha/std::sin(alpha);

            return result;
        }

    };

    inline void ApproximateCircle(std::vector<Vector2>& output, std::span<const SliderControlPoint> controlPoints)
    {
        std::optional<CircularArc> arc = CircularArc::fromControlPoints(controlPoints);
        if (!arc) return ApproximateBezier(output, controlPoints);

        arc->Approximate(output);
    }

    inline void AppendLinear(std::vector<Vector2>& path, std::span<const SliderControlPoint> controlPoints)
    {
        path.reserve(path.size()+controlPoints.size());
        for(const auto& point : controlPoints)
        {
            path.push_back(point.position);
        }
    }


    inline void calculate_segment_path(std::vector<Vector2>& path, std::span<const SliderControlPoint> control_points)
    {
        if (control_points.empty()) return;

        switch(control_points[0].new_slider_type)
        {
            case slider_type::Bezier:
                return ApproximateBezier(path, control_points);
            case slider_type::PerfectCircle:
                return ApproximateCircle(path, control_points);
            case slider_type::Linear:
                return AppendLinear(path, control_points);
            case slider_type::CentripetalCatmullRom:
                return ApproximateCatmull(path, control_points);
            default:
                throw std::runtime_error("unknown slider type");
        }
    }

}
