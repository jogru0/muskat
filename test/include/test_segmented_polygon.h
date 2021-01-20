#pragma once

#include <vector>

#include <gtest/gtest.h>

#include <cool/vec.h>

#include "cyclic_array.h"
#include "errors.h"
#include "global_config.h"
#include "input_preprocessing.h"
#include "segment.h"
#include "segmented_polygon.h"

using Point = alg::Vec2d;
using P = Point;

// ========================================================
// ==         test_create_intermediate_profile           ==
// ========================================================
template<typename Point>
[[nodiscard]] auto test_create_intermediate_profile(const std::string& filename)
	-> turn::SegmentedPolygon<Point>
{
	auto drawing_json = turn::read_json_from_file("./testsuite/testfiles/" + filename + ".json");
	auto tp = turn::TurningProfile<Point>(drawing_json);
	auto final_profile = turn::SegmentedPolygon<Point>::final_profile(tp.points);
	auto monotonized_profile = create_monotonized_profile(final_profile, tp.attrs);
	return create_removable_area(final_profile, monotonized_profile, config::bar_technology).inner_poly;
}

TEST(Test_create_intermediate_profile, NoSkips) {
	auto intermediate = test_create_intermediate_profile<Point>("test_create_intermediate_profile_no_skips");

	//[[4, 5, 6, 7], [7, 8, 9], [9, 10], [10, 11, 12], [12, 13, 14], [14, 0], [0, 1, 2], [2, 3, 4]]
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{50., 52.5}, P{60., 32.5}, P{85., 25.}, P{115., 12.5}};
	expected[turn::seg_id::b] = std::vector{P{115., 12.5}, P{145., 12.5}, P{175., 12.5}};
	expected[turn::seg_id::br] = std::vector{P{175., 12.5}, P{200., 35.}};
	expected[turn::seg_id::r] = std::vector{P{200., 35.}, P{200., 65.}, P{200., 92.5}};
	expected[turn::seg_id::tr] = std::vector{P{200., 92.5}, P{140., 102.5}, P{115., 132.5}};
	expected[turn::seg_id::t] = std::vector{P{115., 132.5}, P{90., 132.5}};
	expected[turn::seg_id::tl] = std::vector{P{90., 132.5}, P{70., 117.5}, P{50., 92.5}};
	expected[turn::seg_id::l] = std::vector{P{50., 92.5}, P{50., 72.5}, P{50., 52.5}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(intermediate[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	}
}

TEST(Test_create_intermediate_profile, Triangle) {
	auto intermediate = test_create_intermediate_profile<Point>("test_create_intermediate_profile_triangle");

	//[[2], [2], [2, 0], [0], [0, 1], [1], [1, 2], [2]]
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{0., 0.}};
	expected[turn::seg_id::b] = std::vector{P{0., 0.}};
	expected[turn::seg_id::br] = std::vector{P{0., 0.}, P{60., 20.}};
	expected[turn::seg_id::r] = std::vector{P{60., 20.}};
	expected[turn::seg_id::tr] = std::vector{P{60., 20.}, P{30., 67.5}};
	expected[turn::seg_id::t] = std::vector{P{30., 67.5}};
	expected[turn::seg_id::tl] = std::vector{P{30., 67.5}, P{0., 0.}};
	expected[turn::seg_id::l] = std::vector{P{0., 0.}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(intermediate[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	} 
}

TEST(Test_create_intermediate_profile, AllSkips) {
	auto intermediate = test_create_intermediate_profile<Point>("test_create_intermediate_profile_all_skips");

	//[[1], [1, 2], [2, 3, 15, 5, 6, 7, 10, 11, 16, 13], [13], [13], [13, 14], [14], [14, 0, 1]]
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{20., 12.5}};
	expected[turn::seg_id::b] = std::vector{P{20., 12.5}, P{55., 12.5}};
	expected[turn::seg_id::br] = std::vector{P{55., 12.5}, P{80., 30.}, P{80., 47.04545454545455}, P{115., 55.}, P{115., 72.5},
											 P{120., 82.5}, P{120., 110.}, P{135., 120.}, P{135., 135.3125}, P{150., 140.}};
	expected[turn::seg_id::r] = std::vector{P{150., 140.}};
	expected[turn::seg_id::tr] = std::vector{P{150., 140.}};
	expected[turn::seg_id::t] = std::vector{P{150., 140.}, P{20., 140.}};
	expected[turn::seg_id::tl] = std::vector{P{20., 140.}};
	expected[turn::seg_id::l] = std::vector{P{20., 140.}, P{20., 47.5}, P{20., 12.5}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(intermediate[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	} 
}

TEST(Test_create_intermediate_profile, NeighbouredSkips) {
	auto intermediate = test_create_intermediate_profile<Point>("test_create_intermediate_profile_neighboured_skips");

	//[[12, 0], [0, 1], [1], [1], [1, 2, 4, 7, 10, 11], [11, 12], [12], [12]]
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P(15., 105.), P(20., 10.)};
	expected[turn::seg_id::b] = std::vector{P(20., 10.), P(140., 10.)};
	expected[turn::seg_id::br] = std::vector{P(140., 10.)};
	expected[turn::seg_id::r] = std::vector{P(140., 10.)};
	expected[turn::seg_id::tr] = std::vector{P(140., 10.), P(125., 47.5), P(95., 47.5), P(65., 47.5), P(65., 85.), P(65., 105.)};
	expected[turn::seg_id::t] = std::vector{P(65., 105.), P(15., 105.)};
	expected[turn::seg_id::tl] = std::vector{P(15., 105.)};
	expected[turn::seg_id::l] = std::vector{P(15., 105.)};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(intermediate[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	} 
}

TEST(Test_create_intermediate_profile, ImpossiblePerforationI) {
	ASSERT_THROW(
		test_create_intermediate_profile<Point>("test_create_intermediate_profile_impossible_perforation"),
		turn::NoSequenceError
	);
}

TEST(Test_create_intermediate_profile, ImpossiblePerforationII) {
	ASSERT_THROW(
		test_create_intermediate_profile<Point>("test_create_intermediate_profile_impossible_perforation_II"),
		turn::NoSequenceError
	);
}

// ========================================================
// ==              test_aabb_outer_profile               ==
// ========================================================
template<typename Point>
[[nodiscard]] auto test_aabb_outer_profile(const std::string& filename)
	-> turn::SegmentedPolygon<Point>
{
	auto inner_profile = test_create_intermediate_profile<Point>(filename);

	auto aabb = turn::get_aabb_poly(inner_profile);
	return turn::detail::create_removable_area_with_aabb_bar(std::move(inner_profile), aabb).outer_poly;
}

TEST(Test_aabb_around_final_profile, General) {
	auto aabb = test_aabb_outer_profile<Point>("test_aabb_outer_profile_general");
	
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{20., 22.5}};
	expected[turn::seg_id::b] = std::vector{P{20., 22.5}, P{80., 22.5}};
	expected[turn::seg_id::br] = std::vector{P{80., 22.5}, P{110., 22.5}, P{135., 22.5}, P{145., 22.5}, P{145., 32.5}, P{145., 102.5}, P{145., 122.5}, P{145., 130.}};
	expected[turn::seg_id::r] = std::vector{P{145, 130.0}};
	expected[turn::seg_id::tr] = std::vector{P{145, 130.0}};
	expected[turn::seg_id::t] = std::vector{P{145, 130.0}};
	expected[turn::seg_id::tl] = std::vector{P{145, 130.0}, P{20, 130.0}, P{20, 100.0}};
	expected[turn::seg_id::l] = std::vector{P{20, 100.0}, P{20, 85.0}, P{20, 67.5}, P{20, 50.0}, P{20, 22.5}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(aabb[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	}
}

TEST(Test_aabb_around_final_profile, Triangle) {
	auto aabb = test_aabb_outer_profile<Point>("test_aabb_outer_profile_triangle");
	
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{0., 0.}};
	expected[turn::seg_id::b] = std::vector{P{0., 0.}};
	expected[turn::seg_id::br] = std::vector{P{0., 0.}, P{60., 0.}, P{60., 20.}};
	expected[turn::seg_id::r] = std::vector{P{60., 20.}};
	expected[turn::seg_id::tr] = std::vector{P{60., 20.}, P{60., 67.5}, P{30., 67.5}};
	expected[turn::seg_id::t] = std::vector{P{30., 67.5}};
	expected[turn::seg_id::tl] = std::vector{P{30., 67.5}, P{0., 67.5}, P{0., 0.}};
	expected[turn::seg_id::l] = std::vector{P{0., 0.}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(aabb[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	} 
}

TEST(Test_aabb_around_final_profile, Sides) {
	auto aabb = test_aabb_outer_profile<Point>("test_aabb_outer_profile_sides");
	
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{20., 10.}};
	expected[turn::seg_id::b] = std::vector{P{20., 10.}, P{45., 10.}, P{80., 10.}, P{100., 10.}};
	expected[turn::seg_id::br] = std::vector{P{100., 10.}, P{120., 10.}, P{120., 22.5}};
	expected[turn::seg_id::r] = std::vector{P{120., 22.5}, P{120., 47.5}, P{120., 62.5}, P{120., 87.5}, P{120., 115.}};
	expected[turn::seg_id::tr] = std::vector{P{120., 115.}};
	expected[turn::seg_id::t] = std::vector{P{120., 115.}, P{75., 115.}, P{50., 115.}, P{20., 115.}};
	expected[turn::seg_id::tl] = std::vector{P{20., 115.}};
	expected[turn::seg_id::l] = std::vector{P{20., 115.}, P{20., 82.5}, P{20., 55.}, P{20., 10.}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(aabb[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	} 
}

// ========================================================
// ==        test_aabb_rotational_with_margins           ==
// ========================================================
TEST(Test_aabb_around_final_profile, rotational_simple) {
	auto original = turn::SegmentedPolygon<Point>::final_profile(std::vector{
			P{0., 1.},P{1., 1.}, P{1.,2.}, P{0., 2.}
	});

	auto aabb = turn::create_removable_area_bart(original, 0., 0., 0.).outer_poly;
	
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{0, 1}, P{0, 0}};
	expected[turn::seg_id::b] = std::vector{P{0, 0}, P{1, 0}};
	expected[turn::seg_id::br] = std::vector{P{1, 0}, P{1, 1}};
	expected[turn::seg_id::r] = std::vector{P{1, 1}, P{1, 2}};
	expected[turn::seg_id::tr] = std::vector{P{1, 2}};
	expected[turn::seg_id::t] = std::vector{P{1, 2}, P{0, 2}};
	expected[turn::seg_id::tl] = std::vector{P{0, 2}};
	expected[turn::seg_id::l] = std::vector{P{0, 2}, P{0, 1}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(aabb[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	}
}

TEST(Test_aabb_around_final_profile, rotational_simple_with_margin) {
	auto original = turn::SegmentedPolygon<Point>::final_profile(std::vector{
			P{0., 1.},P{1., 1.}, P{1.,2.}, P{0., 2.}
	});

	auto aabb = turn::create_removable_area_bart(original, 1., 1., 1.).outer_poly;
	
	auto expected = stdc::cyclic_array<turn::Segment<Point>, turn::seg_id::size>{};
	expected[turn::seg_id::bl] = std::vector{P{-1, 1}, P{-1, 0}, P{0, 0}};
	expected[turn::seg_id::b] = std::vector{P{0, 0}, P{1, 0}};
	expected[turn::seg_id::br] = std::vector{P{1, 0}, P{2, 0}, P{2, 1}};
	expected[turn::seg_id::r] = std::vector{P{2, 1}, P{2, 2}};
	expected[turn::seg_id::tr] = std::vector{P{2, 2}, P{2, 3}, P{1, 3}};
	expected[turn::seg_id::t] = std::vector{P{1, 3}, P{0, 3}};
	expected[turn::seg_id::tl] = std::vector{P{0, 3}, P{-1, 3}, P{-1, 2}};
	expected[turn::seg_id::l] = std::vector{P{-1, 2}, P{-1, 1}};

	for (size_t seg_id = 0; seg_id < turn::seg_id::size; seg_id++) {
		ASSERT_EQ(aabb[turn::SegId{seg_id}], expected[turn::SegId{seg_id}]);
	}
}