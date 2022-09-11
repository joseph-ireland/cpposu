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

    REQUIRE(beatmap.metadata);
    CHECK(beatmap.metadata->at("Title") == "osu! tutorial");
    CHECK(beatmap.metadata->at("Creator") == "peppy");

    REQUIRE(beatmap.timing_points.points.size() == 1);
    CHECK(beatmap.timing_points.points[0].time == 243);
    CHECK(beatmap.timing_points.points[0].beatLength == Approx(374.1233));
    CHECK(beatmap.timing_points.points[0].meter == 4);
    CHECK(beatmap.timing_points.points[0].sampleSet == 1);
    CHECK(beatmap.timing_points.points[0].sampleIndex == 0);
    CHECK(beatmap.timing_points.points[0].volume == 100);
    CHECK(beatmap.timing_points.points[0].uninherited == 1);
    CHECK(beatmap.timing_points.points[0].effects == 0);

    REQUIRE(beatmap.hit_objects.size() == 9);
    using cpposu::HitObject;
    CHECK(beatmap.hit_objects[0] == HitObject{.type=cpposu::circle, .x=64, .y=280, .time=30172});
    CHECK(beatmap.hit_objects[1] == HitObject{.type=cpposu::circle, .x=192, .y=280, .time=31669});
    CHECK(beatmap.hit_objects[2] == HitObject{.type=cpposu::circle, .x=328, .y=280, .time=33165});
    CHECK(beatmap.hit_objects[3] == HitObject{.type=cpposu::circle, .x=456, .y=280, .time=34662});
    CHECK(beatmap.hit_objects[4] == HitObject{.type=cpposu::slider_head, .x=72, .y=192, .time=84046});
    CHECK(beatmap.hit_objects[5] == HitObject{.type=cpposu::slider_head, .x=440, .y=272, .time=87039});
    CHECK(beatmap.hit_objects[6] == HitObject{.type=cpposu::slider_head, .x=136, .y=352, .time=90032});
    CHECK(beatmap.hit_objects[7] == HitObject{.type=cpposu::spinner_start, .x=256, .y=192, .time=113976});
    CHECK(beatmap.hit_objects[8] == HitObject{.type=cpposu::spinner_end, .x=256, .y=192, .time=119587});

}
