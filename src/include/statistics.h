#pragma once

#include <vector>

#include <stdc/algorithm.h>
#include <cmath>
#include <cassert>

namespace stdc{

inline void display_statistics(std::vector<double> data) {
	auto size = data.size();
	assert(2 <= size);
	
	auto sum = std::accumulate(RANGE(data), 0.);
	auto sample_mean = sum / static_cast<double>(size);

	auto sum_of_squared_distances = stdc::transform_accumulate(RANGE(data), [&](auto x) {
		auto dist = std::abs(x - sample_mean);
		return dist * dist;
	});
	auto unbiased_sample_variance = sum_of_squared_distances / static_cast<double>(size - 1);
	auto corrected_sample_standard_deviation = std::sqrt(unbiased_sample_variance);

	auto index_median_l = (size - 1) / 2;
	auto index_median_r = size / 2;

	std::nth_element(data.begin(), stdc::to_it(data, index_median_l), data.end());
	auto median_l = data[index_median_l];
	
	std::nth_element(data.begin(), stdc::to_it(data, index_median_r), data.end());
	auto median_r = data[index_median_l];

	auto sample_median = std::midpoint(median_l, median_r);

	stdc::log("\tmean:     {:.0f}", sample_mean);
	// stdc::log("\tstddev:   {}", corrected_sample_standard_deviation);
	stdc::log("\tmedian:   {:.0f}", sample_median);
	
}


} //namespace stdc