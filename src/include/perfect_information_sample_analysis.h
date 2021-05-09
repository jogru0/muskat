#pragma once

#include <array>
#include <vector>

#include <stdc/literals.h>
#include <stdc/utility.h>
#include <stdc/mathematics.h>

#include <fmt/core.h>
#include <fmt/color.h>

#include "cards.h"

namespace muskat {

	using PerfectInformationResult = std::array<uint8_t, 32>;
	class PerfectInformationSample {
	private:
		Cards m_playable_cards;
		std::vector<PerfectInformationResult> m_points_for_situations;
	public:
		[[nodiscard]] auto playable_cards() const {
			return m_playable_cards;
		}
		[[nodiscard]] auto points_for_situations() const {
			return m_points_for_situations;
		}

		PerfectInformationSample(
			Cards playable_cards,
			std::vector<PerfectInformationResult> &&points_for_situations
		) :
			m_playable_cards{std::move(playable_cards)},
			m_points_for_situations{std::move(points_for_situations)}
		{
			using namespace stdc::literals;
			assert(!m_playable_cards.empty());
			
			for (const auto &points : m_points_for_situations) {
				for (auto i = 0_z; i < 32; ++i) {
					assert(points[i] <= 121);
					assert((points[i] < 121) == (m_playable_cards.contains(static_cast<Card>(i))));
				}
			}
		}
	};

	[[nodiscard]] inline auto to_sample(
		std::vector<PerfectInformationResult> &&points_for_situations
	) {
		using namespace stdc::literals;
		
		assert(!points_for_situations.empty());
		auto cards = Cards{};
		for (auto i = 0_z; i < 32; ++i) {
			if (points_for_situations.front()[i] != 121) {
				cards.add(static_cast<Card>(i));
			}
		}

		return PerfectInformationSample{
			cards, std::move(points_for_situations)
		};
	}

	template<typename PointsToSummand>
	[[nodiscard]] auto get_additive_scores(
		const PerfectInformationSample &sample,
		PointsToSummand summand
	) {
		using namespace stdc::literals;
		
		using Score = std::invoke_result_t<PointsToSummand, uint8_t>;
		static_assert(std::is_same_v<Score, size_t>);

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards = sample.playable_cards();

		auto scores = std::vector<Score>{};
		scores.reserve(cards.size());
		while (!cards.empty()) {
			auto card = cards.remove_next();
			
			auto score = stdc::transform_accumulate(RANGE(points_for_situations), [&] (auto &points){
				return summand(points[static_cast<uint8_t>(card)]);
			});
			scores.push_back(score);
		}
		return scores;
	}

	template<typename Predicate>
	[[nodiscard]] auto get_propability(
		const PerfectInformationSample &sample,
		Predicate predicate
	) {
		using namespace stdc::literals;
		
		using Bool = std::invoke_result_t<Predicate, uint8_t>;
		static_assert(std::is_same_v<Bool, bool>);

		auto sums = get_additive_scores(
			sample,
			[&](auto points) {
				return static_cast<size_t>(predicate(points));
			}
		);

		auto size = sample.points_for_situations().size();

		return stdc::transformed_vector(RANGE(sums), [&](auto sum) {
			return static_cast<double>(sum) / static_cast<double>(size);
		});
	}

	[[nodiscard]] inline auto get_averages(
		const PerfectInformationSample &sample
	) {
		auto sums = get_additive_scores(
			sample,
			stdc::static_cast_to<size_t>{}
		);

		auto size = sample.points_for_situations().size();

		return stdc::transformed_vector(RANGE(sums), [&](auto sum) {
			return static_cast<double>(sum) / static_cast<double>(size);
		});
	}


	template<typename PointsToSummand>
	[[nodiscard]] auto highest_additive_score(
		const PerfectInformationSample &sample,
		PointsToSummand summand,
		Cards cards_to_consider
	) {
		using namespace stdc::literals;
		
		using Score = std::invoke_result_t<PointsToSummand, uint8_t>;
		static_assert(std::is_same_v<Score, size_t>);

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards_to_score = std::vector<std::pair<Card, Score>>{};
		cards_to_score.reserve(cards_to_consider.size());
		auto high_score = Score{};
		while (!cards_to_consider.empty()) {
			auto card = cards_to_consider.remove_next();
			assert(sample.playable_cards().contains(card));
			

			auto score = stdc::transform_accumulate(RANGE(points_for_situations), [&] (auto &points){
				return summand(points[static_cast<uint8_t>(card)]);
			});

			stdc::maximize(high_score, score);
			cards_to_score.emplace_back(card, score);
		}

		auto result = Cards{};
		for (const auto &[c, s] : cards_to_score) {
			if (s == high_score) {
				result.add(c);
			}
		}
		return result;
	}

	template<typename PointsToSummandHead, typename ...PointsToSummandTail>
	[[nodiscard]] auto highest_additive_score(
		const PerfectInformationSample &sample,
		PointsToSummandHead summand_head,
		PointsToSummandTail ...summand_tail
	) {
		
		auto result = highest_additive_score(
			sample,
			summand_head,
			sample.playable_cards()
		);

		(..., (result = highest_additive_score(
			sample,
			summand_tail,
			result
		)));

		return result;
	}

	[[nodiscard]] inline auto analyze_for_declarer(
		const PerfectInformationSample &sample,
		uint8_t threshold
	) {
		return highest_additive_score(
			sample,
			[&](auto points) {
				return static_cast<size_t>(points >= threshold);
			},
			[&](auto points) {
				return static_cast<size_t>(points);
			}
		);
	}

	[[nodiscard]] inline auto analyze_for_defender(
		const PerfectInformationSample &sample,
		uint8_t threshold
	) {
		return highest_additive_score(
			sample,
			[&](auto points) {
				return static_cast<size_t>(points < threshold);
			},
			[&](auto points) {
				return static_cast<size_t>(120 - points);
			}
		);
	}

	[[nodiscard]] inline auto missing_for(uint8_t target, uint8_t current_score) {
		return current_score < target
			? static_cast<uint8_t>(target - current_score)
			: uint8_t{};
	}

	[[nodiscard]] inline auto analyze(
		const PerfectInformationSample &sample,
		uint8_t current_score,
		Role active_role 
	) {
		
		//We primarily optimize proability getting over/staying under this threshold.
		auto threshold = missing_for(61, current_score);

		return active_role == Role::Declarer
			? analyze_for_declarer(sample, threshold)
			: analyze_for_defender(sample, threshold);
	}


	struct cmp_to_helper{
		uint8_t threshold;
		Role active_role;

		[[nodiscard]] auto operator()(uint8_t score) -> bool {
			return active_role == Role::Declarer
				? threshold <= score
				: score < threshold;
		}
	};

	enum class Side {Left, Right};

	template<Side side>
	[[nodiscard]] inline auto stretch_to(
		std::string_view str,
		size_t length
	) {
		
		//We hope to not clip the string.
		assert(str.size() <= length);
		
		auto one_after_last_to_copy = length;
		stdc::minimize(one_after_last_to_copy, str.size());

		auto remaining = length - one_after_last_to_copy;
		auto middle_it = stdc::to_it(str, one_after_last_to_copy);

		auto result = std::string{};
		result.reserve(length);
		
		auto op_str = [&](){
			std::copy(
				str.begin(),
				middle_it,
				std::back_inserter(result)
			);
		};

		auto op_empty = [&](){
			std::fill_n(std::back_inserter(result), remaining, ' ');
		};

		if constexpr (side == Side::Left) {
			op_str();
			op_empty();
		} else {
			op_empty();
			op_str();
		}

		return result;
	}


	template<typename Data, typename Format>
	void print_statistics(
		std::string_view category_string,
		const std::vector<Data> &datas,
		Format format,
		Data highlighted_data
	) {
		using namespace stdc::literals;
		
		assert(category_string.size() < 5);


		std::cout << stretch_to<Side::Left>(category_string, 4) << " | ";
		for (auto id = 0_z; id < datas.size(); ++id) {
			const auto &data = datas[id];
			auto out = stretch_to<Side::Right>(format(data), 6) + " ";
			
			if (data == highlighted_data){
				fmt::print(fmt::emphasis::bold, out);
			} else {
				fmt::print(out);
			}
		}

		std::cout << '\n';
	}

		inline constexpr auto unmodified_threshold_win = 61;
		inline constexpr auto unmodified_threshold_win_schneider = 90;
		inline constexpr auto unmodified_threshold_not_lost_schneider = 31;

	[[nodiscard]] inline auto show_statistics(
		const PerfectInformationSample &sample,
		uint8_t current_score,
		Role active_role,
		Card highlighted_card
	) {
		using namespace stdc::literals;
		
		auto threshold_win = missing_for(unmodified_threshold_win, current_score);
		auto threshold_win_schneider = missing_for(unmodified_threshold_win_schneider, current_score);
		auto threshold_not_lost_schneider = missing_for(unmodified_threshold_not_lost_schneider, current_score);

		auto cmp_to = [&](auto threshold) {
			return cmp_to_helper{threshold, active_role};
		};
		auto cmp_str = active_role == Role::Declarer ? ""s : "<"s;

		auto probabilities_win = get_propability(
			sample,
			cmp_to(threshold_win)
		);
		auto probabilities_win_schneider = get_propability(
			sample,
			cmp_to(threshold_win_schneider)
		);
		auto probabilities_not_lost_schneider = get_propability(
			sample,
			cmp_to(threshold_not_lost_schneider)
		);
		
		auto average_scores = get_averages(sample);

		auto to_percent = [](auto p) {
			return fmt::format("{:.1f}%", 100 * p);
		};
		
		auto to_average_score = [&](auto sc) {
			return fmt::format("{:.2f}", sc + current_score);
		};

		auto to_card = [](auto card) {
			return to_string(card);
		};

		std::cout << "Current Score: " << std::to_string(current_score) << "\n\n";

		print_statistics(
			cmp_str + std::to_string(unmodified_threshold_not_lost_schneider),
			probabilities_not_lost_schneider,
			to_percent,
			*std::max_element(RANGE(probabilities_not_lost_schneider))
		);
		print_statistics(
			cmp_str + std::to_string(unmodified_threshold_win),
			probabilities_win,
			to_percent,
			*std::max_element(RANGE(probabilities_win))
		);
		print_statistics(
			cmp_str + std::to_string(unmodified_threshold_win_schneider),
			probabilities_win_schneider,
			to_percent,
			*std::max_element(RANGE(probabilities_win_schneider))
		);

		auto hightlight_value_average = active_role == Role::Declarer
			? *std::max_element(RANGE(average_scores))
			: *std::min_element(RANGE(average_scores));

		print_statistics(
			"avg.",
			average_scores,
			to_average_score,
			hightlight_value_average
		);

		auto cards = to_vector(sample.playable_cards());

		std::cout << "--------------------------------------------------------\n";
		print_statistics(
			std::string{},
			cards,
			to_card,
			highlighted_card
		);
		



	}


} // namespace muskat
