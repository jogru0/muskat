#pragma once

#include <array>
#include <vector>

#include <stdc/literals.h>
#include <stdc/utility.h>
#include <stdc/mathematics.h>

#include <fmt/core.h>
#include <fmt/color.h>

#include "contract.h"
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
			
			assert(std::all_of(RANGE(m_points_for_situations), [&](const auto &points) {
				for (auto i = 0_z; i < 32; ++i) {
					if (!((points[i] < 121) == (m_playable_cards.contains(static_cast<Card>(i))))) {
						return false;
					}
				}
				return true;
			}));
		}
	};

	template<typename ToSummand>
	[[nodiscard]] auto get_additive_scores(
		const PerfectInformationSample &sample,
		const std::vector<int> &spitzen,
		ToSummand to_summand
	) {
		using namespace stdc::literals;
		
		using Score = std::invoke_result_t<ToSummand, uint8_t, int>;
		static_assert(std::is_same_v<Score, int>);

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards = sample.playable_cards();

		auto scores = std::vector<Score>{};
		scores.reserve(cards.size());
		while (!cards.empty()) {
			auto card = cards.remove_next();
			
			auto score = std::transform_reduce(RANGE(points_for_situations), spitzen.begin(), Score{}, std::plus{}, [&] (auto &points, auto sp){
				return to_summand(points[static_cast<uint8_t>(card)], sp);
			});

			scores.push_back(score);
		}
		return scores;
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

	template<typename Sc>
	[[nodiscard]] inline auto get_propability(
		const std::vector<Sc> &sums_via_bool_predicate,
		size_t sample_size
	) {
		return stdc::transformed_vector(RANGE(sums_via_bool_predicate), [&](auto sum) {
			return static_cast<double>(sum) / static_cast<double>(sample_size);
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

	template<typename Score>
	[[nodiscard]] inline auto get_all_cards_with_score(
		Score target_score,
		const std::vector<Score> &scores,
		Cards parallel_cards
	) {
		using namespace stdc::literals;
		
		auto result = Cards{};
		for (auto i = 0_z; i < scores.size(); ++i) {
			assert(!parallel_cards.empty());
			auto c = parallel_cards.remove_next();
			if (scores[i] == target_score) {
				result.add(c);
			}
		}
		assert(parallel_cards.empty());
		return result;
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



	[[nodiscard]] inline auto get_accumulated_game_results(
		const PerfectInformationSample &sample,
		const std::vector<int> &spitzen,
		Contract contract,
		int bidding_value,
		uint8_t current_score
	) {
		return get_additive_scores(
			sample,
			spitzen,
			[&](auto future_points_and_skat, auto sp) {
				//TODO: Correct number of tricks.
				auto game_result = get_game_result(
					contract,
					sp,
					bidding_value,
					Score{static_cast<uint8_t>(future_points_and_skat + current_score), 5}
				);
				return score_classical(game_result);
			}
		);
	}


	[[nodiscard]] inline auto analyze_new(
		const PerfectInformationSample &sample,
		const std::vector<int> &spitzen,
		Contract contract,
		int bidding_value,
		uint8_t current_score,
		Role active_role
	) {
		auto scores = get_accumulated_game_results(
			sample,
			spitzen,
			contract,
			bidding_value,
			current_score
		);

		auto best_score = active_role == Role::Declarer
			? *std::max_element(RANGE(scores))
			: *std::min_element(RANGE(scores));


		return get_all_cards_with_score(
			best_score,
			scores,
			sample.playable_cards()
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

		[[nodiscard]] auto operator()(uint8_t score) -> size_t {
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


	template<typename Fun, typename Data, typename Format>
	void print_statistics(
		std::string_view category_string,
		const std::vector<Data> &datas,
		Format format,
		Fun is_highlighted
	) {
		using namespace stdc::literals;
		
		assert(category_string.size() < 5);


		std::cout << stretch_to<Side::Left>(category_string, 4) << " | ";
		for (auto id = 0_z; id < datas.size(); ++id) {
			const auto &data = datas[id];
			auto out = stretch_to<Side::Right>(format(data), 7) + " ";
			
			if (is_highlighted(data)){
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
		uint8_t current_score_without_skat,
		Role active_role,
		Cards highlighted_cards,
		const std::vector<int> &spitzen,
		Contract contract,
		int bidding_value
	) {
		using namespace stdc::literals;
		
		auto threshold_win = missing_for(unmodified_threshold_win, current_score_without_skat);
		auto threshold_win_schneider = missing_for(unmodified_threshold_win_schneider, current_score_without_skat);
		auto threshold_not_lost_schneider = missing_for(unmodified_threshold_not_lost_schneider, current_score_without_skat);

		auto size_t_cmp_to = [&](auto threshold) {
			return cmp_to_helper{threshold, active_role};
		};
		auto cmp_str = active_role == Role::Declarer ? ""s : "<"s;

		auto sample_size = sample.points_for_situations().size();

		auto probabilities_win = get_propability(
			get_additive_scores(sample, size_t_cmp_to(threshold_win)),
			sample_size
		);
		auto probabilities_win_schneider = get_propability(
			get_additive_scores(sample, size_t_cmp_to(threshold_win_schneider)),
			sample_size
		);
		auto probabilities_not_lost_schneider = get_propability(
			get_additive_scores(sample, size_t_cmp_to(threshold_not_lost_schneider)),
			sample_size
		);
		
		auto average_scores = get_averages(sample);
		std::transform(RANGE(average_scores), average_scores.begin(), [&](auto score) {
			return score + current_score_without_skat;
		});
		if (active_role != Role::Declarer) {
			std::transform(RANGE(average_scores), average_scores.begin(), [](auto score) {
				return 120. - score;
			});
		}

		auto average_game_results = get_propability(
			get_accumulated_game_results(
				sample,
				spitzen,
				contract,
				bidding_value,
				current_score_without_skat
			),
			sample_size
		);

		auto to_percent = [](auto p) {
			return fmt::format("{:.1f}%", 100 * p);
		};
		
		auto to_average_score = [&](auto sc) {
			return fmt::format("{:.2f}", sc);
		};

		auto to_card = [](auto card) {
			return to_string(card);
		};

		//TODO: Relevant score …
		// std::cout << "Current Score: " << std::to_string(current_score_without_skat) << "\n\n";

		auto max_probabilities_not_lost_schneider = *std::max_element(RANGE(probabilities_not_lost_schneider));
		print_statistics(
			cmp_str + std::to_string(unmodified_threshold_not_lost_schneider),
			probabilities_not_lost_schneider,
			to_percent,
			[&](auto p) { return p == max_probabilities_not_lost_schneider; }
		);

		auto max_probabilities_win = *std::max_element(RANGE(probabilities_win));
		print_statistics(
			cmp_str + std::to_string(unmodified_threshold_win),
			probabilities_win,
			to_percent,
			[&](auto p) { return p == max_probabilities_win; }
		);

		auto max_probabilities_win_schneider = *std::max_element(RANGE(probabilities_win_schneider));
		print_statistics(
			cmp_str + std::to_string(unmodified_threshold_win_schneider),
			probabilities_win_schneider,
			to_percent,
			[&](auto p) { return p == max_probabilities_win_schneider; }
		);

		auto max_average_score = *std::max_element(RANGE(average_scores));
		print_statistics(
			"avg.",
			average_scores,
			to_average_score,
			[&](auto p) { return p == max_average_score; }
			
		);

		auto best_game_result = active_role == Role::Declarer
			? *std::max_element(RANGE(average_game_results))
			: *std::min_element(RANGE(average_game_results));

		print_statistics(
			"game",
			average_game_results,
			to_average_score,
			[&](auto p) { return p == best_game_result; }
			
		);

		auto cards = to_vector(sample.playable_cards());

		std::cout << "--------------------------------------------------------\n";
		print_statistics(
			std::string{},
			cards,
			to_card,
			[&](auto card) { return highlighted_cards.contains(card); }
		);
	}

} // namespace muskat
