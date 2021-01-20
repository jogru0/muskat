#pragma once

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <cool/units.h>

#include "enums.h"
#include "global_config.h"
#include "hashed_vector.h"
#include "perforation/perforation_class.h"
#include "simulator.h"
#include "tools.h"

using Point = alg::Vec2d;
using P = Point;

struct Mock_PerforationIsReachableFrom : public turn::PerforationIsReachableFrom {
	using Parent = turn::PerforationIsReachableFrom;

	Mock_PerforationIsReachableFrom(bool left, bool right) :
		Parent{left, right}
	{}
};

template<typename Point>
[[nodiscard]] auto test_get_perforation_from_file(const std::string& filename)
{
	auto drawing_json = turn::read_json_from_file("./testsuite/testfiles/" + filename + ".json");
	auto tp = turn::TurningProfile<Point>(drawing_json);
	auto final_profile = turn::SegmentedPolygon<Point>::final_profile(tp.points);
	auto monotonized_profile = create_monotonized_profile(final_profile, tp.attrs);
	auto removable_area = create_removable_area(final_profile, monotonized_profile, config::bar_technology);
	auto difference_profile = removable_area.inner_poly;
	
	auto perforations_vector = turn::create_all_perforation_vectors(final_profile, difference_profile, tp.attrs);
	EXPECT_TRUE((perforations_vector.size() == 1)); //cannot use ASSERT_TRUE due to it returning void

	auto is_reachable_from = Mock_PerforationIsReachableFrom{true, true};
	return turn::Perforation{std::move(perforations_vector.front()), turn::CardinalDirection::N, is_reachable_from};
}

// =========================================================================
// ==         test_get_possible_perforation_and_subperforations           ==
// =========================================================================

struct Test_PerfCanBeDone {
	Test_PerfCanBeDone(double _blade_width) :
		blade_width{_blade_width}
	{}

	template<typename Point>
	[[nodiscard]] bool operator()(const turn::detail::EndPointsPerf<Point>& perf_points) const
	{
		double perf_width = perforation_width(perf_points).get_value_in_unit(stdc::units::mm);
		return blade_width < perf_width;
	}

	const double blade_width;
};

TEST(Test_get_possible_perforation_and_subperforations, General) {
	const auto perf = test_get_perforation_from_file<Point>("test_perforate_as_much_as_possible_with_given_blade_general");
	// ====
	{
		auto [perf_30, sub_perf_30] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{30});
		ASSERT_FALSE(perf_30);
		ASSERT_TRUE(sub_perf_30.size() == 1);
		ASSERT_EQ(sub_perf_30[0], perf);
	}
	// ====
	{
		auto [perf_20, sub_perf_20] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{20});
		ASSERT_TRUE(perf_20);

		auto possible_perf_20 = turn::Perforation{
			std::vector{P{40., 20.}, P{40., 15.}, P{35., 15.}, P{20., 15.}, P{15., 15.}, P{15., 20.}},
			turn::CardinalDirection::N, perf.is_reachable_from
		};
		auto possible_subperfs_20 = std::vector{
			turn::Perforation{
				std::vector{P{35., 15.}, P{35., 12.5}, P{30., 12.5}, P{30., 10.}, P{25., 10.}, P{25., 12.5}, P{20., 12.5}, P{20., 15.}},
				turn::CardinalDirection::N, perf.is_reachable_from
			}
		};
		ASSERT_EQ(perf_20, possible_perf_20);
		ASSERT_EQ(sub_perf_20, possible_subperfs_20);
	}
	// ====
	{
		auto [perf_10, sub_perf_10] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{10});
		ASSERT_TRUE(perf_10);

		auto possible_perf_10 = turn::Perforation{
			std::vector{P{40., 20.}, P{40., 15.}, P{35., 15.}, P{35., 12.5}, P{30., 12.5}, P{25., 12.5}, P{20., 12.5}, P{20., 15.}, P{15., 15.}, P{15., 20.}},
			turn::CardinalDirection::N, perf.is_reachable_from
		};
		auto possible_subperfs_10 = std::vector{
			turn::Perforation{
				std::vector{P{30., 12.5}, P{30., 10.}, P{25., 10.}, P{25., 12.5}},
				turn::CardinalDirection::N, perf.is_reachable_from
			}
		};
		ASSERT_EQ(perf_10, possible_perf_10);
		ASSERT_EQ(sub_perf_10, possible_subperfs_10);
	}
	// ====
	{
		auto [perf_3, sub_perf_3] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{3});
		ASSERT_TRUE(perf_3);

		auto possible_perf_3 = turn::Perforation{
			std::vector{P{40., 20.}, P{40., 15.}, P{35., 15.}, P{35., 12.5}, P{30., 12.5}, P{30., 10.}, P{25., 10.},
						P{25., 12.5}, P{20., 12.5}, P{20., 15.}, P{15., 15.}, P{15., 20.}},
			turn::CardinalDirection::N, perf.is_reachable_from
		};
		auto possible_subperfs_3 = std::vector<turn::Perforation<Point>>{};
		ASSERT_EQ(perf_3, possible_perf_3);
		ASSERT_EQ(sub_perf_3, possible_subperfs_3);
	}
}

// TEST(Test_get_possible_perforation_and_subperforations, Schraeg) {
// 	const auto perf = test_get_perforation_from_file<Point>("test_perforate_as_much_as_possible_with_given_blade_general_schraeg");
// 	// ====
// 	{
// 		auto [perf_30, sub_perf_30] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{30});
// 		ASSERT_FALSE(perf_30);
// 	}
// 	// ====
// 	{
// 		auto [perf_15, sub_perf_15] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{15});
// 		ASSERT_TRUE(perf_15);

// 		auto possible_perf_15 = turn::Perforation{
// 			std::vector{P{40., 25.}, P{35., 18.5}, P{30., 18.5}, P{15., 18.5}, P{10., 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_15 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{30., 18.5}, P{30., 12.5}, P{25., 10.}, P{20., 10.}, P{20., 12.5}, P{15., 18.5}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_15, possible_perf_15);
// 		ASSERT_EQ(sub_perf_15, possible_subperfs_15);
// 	}
// 	// ====
// 	{
// 		auto [perf_8, sub_perf_8] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{8});
// 		ASSERT_TRUE(perf_8);

// 		auto possible_perf_8 = turn::Perforation{
// 			std::vector{P{40., 25.}, P{35., 18.75}, P{30., 18.75}, P{30., 12.5}, P{20., 12.5}, P{15., 18.75}, P{10., 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_8 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{30., 12.5}, P{25., 10.}, P{20., 10.}, P{20., 12.5}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_8, possible_perf_8);
// 		ASSERT_EQ(sub_perf_8, possible_subperfs_8);
// 	}
// 	// ====
// 	{
// 		auto [perf_3, sub_perf_3] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{3});
// 		ASSERT_TRUE(perf_3);

// 		auto possible_perf_3 = turn::Perforation{
// 			std::vector{P{40., 25.}, P{35., 18.75}, P{30., 18.75}, P{30., 12.5}, P{25., 10.}, P{20., 10.}, P{20., 12.5}, P{15., 18.75}, P{10., 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_3 = std::vector<turn::Perforation<Point>>{};
// 		ASSERT_EQ(perf_3, possible_perf_3);
// 		ASSERT_EQ(sub_perf_3, possible_subperfs_3);
// 	}
// }

// TEST(Test_get_possible_perforation_and_subperforations, ZeroAndRaycast) {
// 	const auto perf = test_get_perforation_from_file<Point>("test_perforate_as_much_as_possible_with_given_blade_zero_and_raycast");
// 	// ====
// 	{
// 		auto [perf_15, sub_perf_15] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{15});
// 		ASSERT_FALSE(perf_15);
// 	}
// 	// ====
// 	{
// 		auto [perf_10, sub_perf_10] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{10});
// 		ASSERT_TRUE(perf_10);

// 		auto possible_perf_10 = turn::Perforation{
// 			std::vector{P{32.5, 20.}, P{28.75, 12.5}, P{17.5, 12.5}, P{15., 20.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_10 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{28.75, 12.5}, P{27.5, 10.}, P{22.5, 8.75}, P{20.833333333333332, 10.}, P{17.5, 12.5}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_10, possible_perf_10);
// 		ASSERT_EQ(sub_perf_10, possible_subperfs_10);
// 	}
// 	// ====
// 	{
// 		auto [perf_5, sub_perf_5] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{5});
// 		ASSERT_TRUE(perf_5);

// 		auto possible_perf_5 = turn::Perforation{
// 			std::vector{P{32.5, 20.}, P{28.75, 12.5}, P{27.5, 10.}, P{20.833333333333332, 10.}, P{17.5, 12.5}, P{15., 20.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_5 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{27.5, 10.}, P{22.5, 8.75}, P{20.833333333333332, 10.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_5, possible_perf_5);
// 		ASSERT_EQ(sub_perf_5, possible_subperfs_5);

// 	// ====
// 		auto [perf_001, sub_perf_001] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{0.01});
// 		auto possible_perf_001 = possible_perf_5;
// 		auto possible_subperfs_001 = possible_subperfs_5;
// 		ASSERT_EQ(perf_001, possible_perf_001);
// 		ASSERT_EQ(sub_perf_001, possible_subperfs_001);
// 	}
// }

// TEST(Test_get_possible_perforation_and_subperforations, DoubleRaycast) {
// 	const auto perf = test_get_perforation_from_file<Point>("test_perforate_as_much_as_possible_with_given_blade_double_raycast");
// 	// ====
// 	{
// 		auto [perf_8, sub_perf_8] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{8});
// 		ASSERT_TRUE(perf_8);

// 		auto possible_perf_8 = turn::Perforation{
// 			std::vector{P{25, 15.}, P{25., 11.25}, P{20., 11.25}, P{17.5, 11.25}, P{15., 11.25}, P{15., 15.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_8 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{25., 11.25}, P{25., 7.5}, P{20., 7.5}, P{20., 11.25}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{17.5, 11.25}, P{17.5, 7.5}, P{15., 7.5}, P{15., 11.25}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_8, possible_perf_8);
// 		ASSERT_EQ(sub_perf_8, possible_subperfs_8);
// 	}
// }

// TEST(Test_get_possible_perforation_and_subperforations, MultipleSubperfs) {
// 	const auto perf = test_get_perforation_from_file<Point>("test_perforate_as_much_as_possible_with_given_blade_multiple_subperforations");
// 	// ====
// 	{
// 		auto [perf_30, sub_perf_30] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{30});
// 		ASSERT_TRUE(perf_30);

// 		auto possible_perf_30 = turn::Perforation{
// 			std::vector{P{50., 25.}, P{50., 20.}, P{40., 20.}, P{37.5, 20.}, P{35., 20.}, P{32.5, 20.}, P{20., 20.}, P{15., 20.}, P{15., 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_30 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{50., 20.}, P{50., 15.}, P{50., 10.}, P{42.5, 10.}, P{42.5, 15.}, P{40., 15.}, P{40., 20.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{37.5, 20.}, P{37.5, 15.}, P{35., 15.}, P{35., 20.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{32.5, 20.}, P{32.5, 15.}, P{30., 15.}, P{30., 10.}, P{27.5, 10.}, P{27.5, 15.},
// 							P{25., 15.}, P{25., 10.}, P{22.5, 10.}, P{22.5, 15.}, P{20., 15.}, P{20., 20.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_30, possible_perf_30);
// 		ASSERT_EQ(sub_perf_30, possible_subperfs_30);
// 	}
	// ====
// 	{
// 		auto [perf_12, sub_perf_12] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{12});
// 		ASSERT_TRUE(perf_12);

// 		auto possible_perf_12 = turn::Perforation{
// 			std::vector{P{50., 25.}, P{50., 20.}, P{40., 20.}, P{37.5, 20.}, P{35., 20.}, P{32.5, 20.}, P{32.5, 15.},
// 						P{30., 15.}, P{27.5, 15.}, P{25., 15.}, P{22.5, 15.}, P{20., 15.}, P{20., 20.}, P{15., 20.}, P{15, 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_12 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{50., 20.}, P{50., 15.}, P{50., 10.}, P{42.5, 10.}, P{42.5, 15.}, P{40., 15.}, P{40., 20.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{37.5, 20.}, P{37.5, 15.}, P{35., 15.}, P{35., 20.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{30., 15.}, P{30., 10.}, P{27.5, 10.}, P{27.5, 15.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{25., 15.}, P{25., 10.}, P{22.5, 10.}, P{22.5, 15.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_12, possible_perf_12);
// 		ASSERT_EQ(sub_perf_12, possible_subperfs_12);
// 	}
// 	// ====
// 	{
// 		auto [perf_8, sub_perf_8] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{8});
// 		ASSERT_TRUE(perf_8);

// 		auto possible_perf_8 = turn::Perforation{
// 			std::vector{P{50., 25.}, P{50., 20.}, P{50., 15.}, P{42.5, 15.}, P{40., 15.}, P{40., 20.}, P{37.5, 20.},
// 						P{35., 20.}, P{32.5, 20.}, P{32.5, 15.}, P{30., 15.}, P{27.5, 15.}, P{25., 15.}, P{22.5, 15.},
// 						P{20., 15.}, P{20., 20.}, P{15., 20.}, P{15, 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_8 = std::vector{
// 			turn::Perforation{
// 				std::vector{P{50., 15.}, P{50., 10.}, P{42.5, 10.}, P{42.5, 15.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{37.5, 20.}, P{37.5, 15.}, P{35., 15.}, P{35., 20.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{30., 15.}, P{30., 10.}, P{27.5, 10.}, P{27.5, 15.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			},
// 			turn::Perforation{
// 				std::vector{P{25., 15.}, P{25., 10.}, P{22.5, 10.}, P{22.5, 15.}},
// 				turn::CardinalDirection::N, perf.is_reachable_from
// 			}
// 		};
// 		ASSERT_EQ(perf_8, possible_perf_8);
// 		ASSERT_EQ(sub_perf_8, possible_subperfs_8);
// 	}
// 	// ====
// 	{
// 		auto [perf_2, sub_perf_2] = perf.get_possible_perforation_and_subperforations(Test_PerfCanBeDone{2});
// 		ASSERT_TRUE(perf_2);

// 		auto possible_perf_2 = turn::Perforation{
// 			std::vector{P{50., 25.}, P{50., 20.}, P{50., 15.}, P{50., 10.}, P{42.5, 10.}, P{42.5, 15.}, P{40., 15.},
// 						P{40., 20.}, P{37.5, 20.}, P{37.5, 15.}, P{35., 15.}, P{35., 20.}, P{32.5, 20.}, P{32.5, 15.},
// 						P{30., 15.}, P{30., 10.}, P{27.5, 10.}, P{27.5, 15.}, P{25., 15.}, P{25., 10.}, P{22.5, 10.},
// 						P{22.5, 15.}, P{20., 15.}, P{20., 20.}, P{15., 20.}, P{15, 25.}},
// 			turn::CardinalDirection::N, perf.is_reachable_from
// 		};
// 		auto possible_subperfs_2 = std::vector<turn::Perforation<Point>>{};
// 		ASSERT_EQ(perf_2, possible_perf_2);
// 		ASSERT_EQ(sub_perf_2, possible_subperfs_2);
// 	}
// }