#include "cpposu/types.hpp"
#include <external/catch2/catch.hpp>

#include <cpposu/beatmap_parser.hpp>



TEST_CASE("parse tutorial", "[beatmap_parser]")
{
    cpposu::BeatmapParser parser(CPPOSU_TEST_DIR "./Peter Lambert - osu! tutorial (peppy) [Gameplay basics].osu");
    auto beatmap = parser.parse();

    CHECK(beatmap.difficulty_attributes.HPDrainRate == Approx(0));
    CHECK(beatmap.difficulty_attributes.CircleSize == Approx(3));
    CHECK(beatmap.difficulty_attributes.OverallDifficulty == Approx(0));
    CHECK(beatmap.difficulty_attributes.SliderMultiplier == Approx(0.6));
    CHECK(beatmap.difficulty_attributes.SliderTickRate == Approx(1));

    CHECK(beatmap.info.Title == "osu! tutorial");
    CHECK(beatmap.info.Creator == "peppy");
    CHECK(beatmap.info.StackLeniency == 0.7f);

    REQUIRE(beatmap.timing_points.points.size() == 1);
    CHECK(beatmap.timing_points.points[0].time == 243);
    CHECK(beatmap.timing_points.points[0].beatLength == Approx(374.1233));
    CHECK(beatmap.timing_points.points[0].meter == 4);
    CHECK(beatmap.timing_points.points[0].sampleSet == 1);
    CHECK(beatmap.timing_points.points[0].sampleIndex == 0);
    CHECK(beatmap.timing_points.points[0].volume == 100);
    CHECK(beatmap.timing_points.points[0].timing_change == 1);
    CHECK(beatmap.timing_points.points[0].effects == 0);

    REQUIRE(beatmap.hit_objects.size() == 32);
    int i=0;
    using cpposu::HitObject;
    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::circle, .x=64, .y=280, .time=30172});
    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::circle, .x=192, .y=280, .time=31669});
    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::circle, .x=328, .y=280, .time=33165});
    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::circle, .x=456, .y=280, .time=34662});

    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::slider_head, .x=72, .y=192, .time=84046});

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(72.0, 432.0, 1.0/6)));
    CHECK(beatmap.hit_objects[i].y == 192);
    CHECK(beatmap.hit_objects[i].time == Approx(84420).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(72.0, 432.0, 2.0/6)));
    CHECK(beatmap.hit_objects[i].y == 192);
    CHECK(beatmap.hit_objects[i].time == Approx(84794).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(72.0, 432.0, 3.0/6)));
    CHECK(beatmap.hit_objects[i].y == 192);
    CHECK(beatmap.hit_objects[i].time == Approx(85168).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(72.0, 432.0, 4.0/6)));
    CHECK(beatmap.hit_objects[i].y == 192);
    CHECK(beatmap.hit_objects[i].time == Approx(85543).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(72.0, 432.0, 5.0/6)));
    CHECK(beatmap.hit_objects[i].y == 192);
    CHECK(beatmap.hit_objects[i].time == Approx(85917).epsilon(0.5));
    i++;
    CHECK(beatmap.hit_objects[i++].type == cpposu::slider_legacy_last_tick);

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tail);
    CHECK(beatmap.hit_objects[i].x == 432);
    CHECK(beatmap.hit_objects[i].y == 192);
    CHECK(beatmap.hit_objects[i].time == Approx(86291).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::slider_head, .x=440, .y=272, .time=87039});

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(440.0, 80.0, 1.0/6)));
    CHECK(beatmap.hit_objects[i].y == 272);
    CHECK(beatmap.hit_objects[i].time == Approx(87413).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(440.0, 80.0, 2.0/6)));
    CHECK(beatmap.hit_objects[i].y == 272);
    CHECK(beatmap.hit_objects[i].time == Approx(87787).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(440.0, 80.0, 3.0/6)));
    CHECK(beatmap.hit_objects[i].y == 272);
    CHECK(beatmap.hit_objects[i].time == Approx(88616).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(440.0, 80.0, 4.0/6)));
    CHECK(beatmap.hit_objects[i].y == 272);
    CHECK(beatmap.hit_objects[i].time == Approx(88536).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(440.0, 80.0, 5.0/6)));
    CHECK(beatmap.hit_objects[i].y == 272);
    CHECK(beatmap.hit_objects[i].time == Approx(88910).epsilon(0.5));
    i++;
    CHECK(beatmap.hit_objects[i++].type == cpposu::slider_legacy_last_tick);

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tail);
    CHECK(beatmap.hit_objects[i].x == Approx(80));
    CHECK(beatmap.hit_objects[i].y == 272);
    CHECK(beatmap.hit_objects[i].time == Approx(89284).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::slider_head, .x=136, .y=352, .time=90032});

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(136.0, 376.0, 1.0/4)));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(90406).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(136.0, 376.0, 2.0/4)));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(90780).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(136.0, 376.0, 3.0/4)));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(91154).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_repeat);
    CHECK(beatmap.hit_objects[i].x == Approx(376));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(91529).epsilon(0.5));
    i++;


    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(136.0, 376.0, 3.0/4)));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(91903).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(136.0, 376.0, 2.0/4)));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(92277).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tick);
    CHECK(beatmap.hit_objects[i].x == Approx(std::lerp(136.0, 376.0, 1.0/4)));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(92651).epsilon(0.5));
    i++;
    CHECK(beatmap.hit_objects[i++].type == cpposu::slider_legacy_last_tick);

    CHECK(beatmap.hit_objects[i].type == cpposu::slider_tail);
    CHECK(beatmap.hit_objects[i].x == Approx(136));
    CHECK(beatmap.hit_objects[i].y == 352);
    CHECK(beatmap.hit_objects[i].time == Approx(93025).epsilon(0.5));
    i++;

    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::spinner_start, .x=256, .y=192, .time=113976});
    CHECK(beatmap.hit_objects[i++] == HitObject{.type=cpposu::spinner_end, .x=256, .y=192, .time=119587});

}
