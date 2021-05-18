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

	using PerfectInformationResult = std::array<Score, 32>;
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
					if (!((points[i].points() < 121) == (m_playable_cards.contains(static_cast<Card>(i))))) {
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
		
		using Sc = std::invoke_result_t<ToSummand, Score, int>;
		static_assert(std::is_same_v<Sc, int>);

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards = sample.playable_cards();

		auto scores = std::vector<Sc>{};
		scores.reserve(cards.size());
		while (!cards.empty()) {
			auto card = cards.remove_next();
			
			auto score = std::transform_reduce(RANGE(points_for_situations), spitzen.begin(), Sc{}, std::plus{}, [&] (auto &points, auto sp){
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
		
		using Sc = std::invoke_result_t<PointsToSummand, Score>;
		static_assert(std::is_same_v<Sc, size_t>);

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards = sample.playable_cards();

		auto scores = std::vector<Sc>{};
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
			[](auto score) { return static_cast<size_t>(score.points()); }
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
		
		using Sc = std::invoke_result_t<PointsToSummand, Score>;
		static_assert(std::is_same_v<Sc, size_t>);

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards_to_score = std::vector<std::pair<Card, Sc>>{};
		cards_to_score.reserve(cards_to_consider.size());
		auto high_score = Sc{};
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
		Score threshold
	) {
		return highest_additive_score(
			sample,
			[&](Score score) {
				return static_cast<size_t>(score.points() >= threshold.points());
			},
			[&](auto score) {
				return static_cast<size_t>(score.points());
			}
		);
	}



	[[nodiscard]] inline auto get_accumulated_game_results(
		const PerfectInformationSample &sample,
		const std::vector<int> &spitzen,
		Contract contract,
		int bidding_value,
		Score current_score
	) {
		return get_additive_scores(
			sample,
			spitzen,
			[&](Score future_points_and_skat, auto sp) {
				//TODO: Correct number of tricks.
				auto final_score = current_score;
				final_score.add(future_points_and_skat);
				auto game_result = get_game_result(
					contract,
					sp,
					bidding_value,
					final_score
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
		Score current_score,
		Role active_role
	) {
		auto scores = get_accumulated_game_results(
			sample,
			spitzen,
			contract,
			bidding_value,
			current_score
		);

		//Here, Null is a maximizer …
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
		Score threshold
	) {
		return highest_additive_score(
			sample,
			[&](Score points) {
				return static_cast<size_t>(points < threshold);
			},
			[&](auto points) {
				return static_cast<size_t>(120 - points.points());
			}
		);
	}

	struct cmp_to_helper{
		Score threshold;
		bool is_maximizer;

		[[nodiscard]] auto operator()(Score score) -> size_t {
			return is_maximizer
				? threshold <= score
				: score < threshold;
		}
	};

	enum class Side {Left, Right};

	template<Side side, typename Str>
	[[nodiscard]] inline auto stretch_to(
		const Str &str,
		size_t length
	) {

		using Char = typename Str::value_type;
		
		//We hope to not clip the string.
		assert(str.size() <= length);
		
		auto one_after_last_to_copy = length;
		stdc::minimize(one_after_last_to_copy, str.size());

		auto remaining = length - one_after_last_to_copy;
		auto middle_it = stdc::to_it(str, one_after_last_to_copy);

		auto result = std::basic_string<Char>{};
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

	inline /*constexpr*/ const auto unmodified_threshold_win = Score{61, 0};
	inline /*constexpr*/ const auto unmodified_threshold_win_schneider = Score{90, 0};
	inline /*constexpr*/ const auto unmodified_threshold_not_lost_schneider = Score{31, 0};
	// There is no threshold for schwarz, as e.g. stuff like {22, 0} can happen.

	[[nodiscard]] inline auto show_statistics(
		const PerfectInformationSample &sample,
		Score current_score_without_skat,
		Role active_role,
		Cards highlighted_cards,
		const std::vector<int> &spitzen,
		Contract contract,
		int bidding_value
	) {
		using namespace stdc::literals;

		auto print_props = [](
			const auto &probs,
			std::string_view category_string
		) {
			auto max_prob = *std::max_element(RANGE(probs));

			if (category_string != "w") {
				if (max_prob == 0.0) {
					// fmt::print(fmt::fg(fmt::color::red), stretch_to<Side::Left>(category_string, 4));
					// std::cout << " |\n";
					return;
				}

				if (*std::min_element(RANGE(probs)) == 1) {
					// fmt::print(fmt::fg(fmt::color::green), stretch_to<Side::Left>(category_string, 4));
					// std::cout << " |\n";
					return;
				}
			}

			print_statistics(
				category_string,
				probs,
				[](auto p) {
					return fmt::format("{:.1f}%", 100 * p);
				},
				[&](auto p) { return p == max_prob; }
			);
		};

		auto size_t_cmp_to = [&](Score threshold) {
			return cmp_to_helper{threshold, is_maximizer(active_role, contract.game)};
		};
		auto sample_size = sample.points_for_situations().size();


		auto vert_line = "------"s;
		for (auto i = 0_z; i < sample.playable_cards().size(); ++i) {
			vert_line += "--------";
		}
		vert_line.push_back('\n');

		auto to_average_score = [&](auto sc) {
			return fmt::format("{:.2f}", sc);
		};

		if (contract.game != GameType::Null) {
			auto threshold_win = required_beyond_to_reach(current_score_without_skat, unmodified_threshold_win);
			auto threshold_win_schneider = required_beyond_to_reach(current_score_without_skat, unmodified_threshold_win_schneider);
			auto threshold_not_lost_schneider = required_beyond_to_reach(current_score_without_skat, unmodified_threshold_not_lost_schneider);

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

			auto probabilities_win_schwarz = get_propability(
				get_additive_scores(sample, [&](auto score) {
					return static_cast<size_t>(score.tricks() + current_score_without_skat.tricks() == (active_role == Role::Declarer ? 10 : 0));
				}),
				sample_size
			);
			auto probabilities_not_lost_schwarz = get_propability(
				get_additive_scores(sample, [&](auto score) {
					return static_cast<size_t>(score.tricks() + current_score_without_skat.tricks() != (active_role == Role::Declarer ? 0 : 10));
				}),
				sample_size
			);
			
			if (active_role != Role::Declarer) {
				std::swap(probabilities_win_schneider, probabilities_not_lost_schneider);
				std::swap(probabilities_win_schwarz, probabilities_not_lost_schwarz);
				std::swap(probabilities_win_schwarz, probabilities_not_lost_schwarz);
			}
			
			auto average_scores = get_averages(sample);
			std::transform(RANGE(average_scores), average_scores.begin(), [&](auto score) {
				return score + current_score_without_skat.points();
			});
			if (active_role != Role::Declarer) {
				std::transform(RANGE(average_scores), average_scores.begin(), [](auto score) {
					return 120. - score;
				});
			}


			//TODO: Relevant score …
			// std::cout << "Current Score: " << std::to_string(current_score_without_skat) << "\n\n";
			

			auto max_average_score = *std::max_element(RANGE(average_scores));
			print_statistics(
				"avg.",
				average_scores,
				to_average_score,
				[&](auto p) { return p == max_average_score; }
				
			);
			
			std::cout << vert_line;

			print_props(probabilities_not_lost_schwarz, "nlb");
			print_props(probabilities_not_lost_schneider, "nls");
			print_props(probabilities_win, "w");
			print_props(probabilities_win_schneider, "ws");
			print_props(probabilities_win_schwarz, "wb");
			
		} else {
			//Null game.
			
			auto probabilities_win = get_propability(
				get_additive_scores(sample, [&](auto score) {
					auto is_win_dec = score.tricks() + current_score_without_skat.tricks() == 0;
					auto is_win = active_role == Role::Declarer ? is_win_dec : !is_win_dec;
					return static_cast<size_t>(is_win);
				}),
				sample_size
			);
			print_props(probabilities_win, "w");
		}
		
		std::cout << vert_line;

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

		//And here, the Null Spieler is a maximizer again …
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

		auto to_card = [](auto card) {
			return to_string(card);
		};

		std::cout << vert_line;
		print_statistics(
			std::string{},
			cards,
			to_card,
			[&](auto card) { return highlighted_cards.contains(card); }
		);
	}

} // namespace muskat
