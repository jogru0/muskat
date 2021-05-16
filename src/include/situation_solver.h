#pragma once

#include "situation.h"

#include "contract.h"

#include <stdc/mathematics.h>

#include <robin_hood/robin_hood.h>

namespace muskat {


	[[nodiscard]] inline auto get_cards_to_consider(
		Situation sit,
		GameType game,
		MaybeCard maybe_preference
	) {
		using namespace stdc::literals;

		auto result = std::array<MaybeCard, 10>{};
		assert(result[0] == MaybeCard{});
		assert(result[1] == MaybeCard{});
		assert(result[9] == MaybeCard{});
		
		auto cards = next_possible_plays(sit, game);
		assert(!cards.empty());
		assert(cards.size() < 11);

		auto next_index = 0_z;
		if (maybe_preference) {
			result[next_index] = *maybe_preference;
			cards.remove(*maybe_preference);
			++next_index;
		}

		auto maybe_forced_tt = [&]() -> std::optional<TrickAndGameType> {
			if (auto maybe_first = sit.get_maybe_first_trick_card()) {
				return TrickAndGameType{*maybe_first, game};
			}
			return std::nullopt;
		}();

		auto n = next(sit.active_role());
		auto nn = next(n);
		auto n_h = sit.hand(n);
		auto nn_h = sit.hand(nn);
		auto get_options = [&](auto card) {
			if (maybe_forced_tt) {
				return 0ul;
			}
			auto tt = TrickAndGameType{card, game};
			return legal_response_cards(n_h, tt).size() * legal_response_cards(nn_h, tt).size();
		};



		while(!cards.empty()) {
			auto cards_mod = cards;
			auto best_card = cards_mod.remove_next();
			auto best_options = get_options(best_card);
			auto best_power = to_power(best_card, maybe_forced_tt.value_or(TrickAndGameType{best_card, game}));
			while (!cards_mod.empty()) {
				auto card = cards_mod.remove_next();
				auto options = get_options(card);
				auto power = to_power(card, maybe_forced_tt.value_or(TrickAndGameType{card, game}));
				
				if (std::tuple{options, -power} < std::tuple{best_options, -best_power}) {
					best_card = card;
					best_options = options;
					best_power = power;
				}
			}

			result[next_index] = best_card;
			cards.remove(best_card);
			++next_index;
		}

		return result;
	}

	class Bounds {
	public:
		Score m_lower;
		Score m_upper;
	public:
		explicit constexpr Bounds(Score lower, Score upper) :
			m_lower{lower}, m_upper{upper}
		{
			assert(m_lower <= m_upper);
		}
		
		//TODO: Where used? Should this be so?
		explicit constexpr Bounds() : Bounds{Score{0, 0}, Score{0, 0}} {}

		[[nodiscard]] constexpr auto lower() const{ return m_lower; }
		[[nodiscard]] constexpr auto upper() const{ return m_upper; }

		constexpr void update_lower(Score new_value) {
			assert (m_lower <= new_value);
			m_lower = new_value;
			assert(m_lower <= m_upper);
		}
		constexpr void update_upper(Score new_value) {
			assert(new_value <= m_upper);
			m_upper = new_value;
			assert(m_lower <= m_upper);
		}
	};

	[[nodiscard]] inline constexpr auto operator==(Bounds left, Bounds right) {
		return left.lower() == right.lower() && left.upper() == right.upper();
	}

	[[nodiscard]] inline constexpr auto decides_threshold(Bounds bounds, Score threshold) {
		return threshold <= bounds.lower() || bounds.upper() < threshold;
	}

	[[nodiscard]] inline auto quick_bounds(Situation sit, GameType game) {
		constexpr auto max_points = 120;
		auto lower = Score{0, 0};
		//TODO: How many tricks left?
		auto upper = Score{static_cast<uint8_t>(max_points - to_points(sit.cellar(), game)), 10};
		return std::pair{Bounds{lower, upper}, MaybeCard{}};
	}

	class SituationSolver {
	private:
		using LookUpTable = robin_hood::unordered_flat_map<
			Cards,
			std::pair<Bounds, MaybeCard>
		>;

		LookUpTable m_look_up;
		GameType m_game;
		Card m_skat_0;
		Card m_skat_1;
		Cards m_h_dec;
		Cards m_h_fd;
		Cards m_h_sd;

		[[nodiscard]] auto &current_bounds(Situation sit) {
			auto key = sit.hand(Role::Declarer) |
				sit.hand(Role::FirstDefender) |
				sit.hand(Role::SecondDefender);
			switch (sit.active_role()) {
				case Role::Declarer: break;
				case Role::FirstDefender: key.add(m_skat_0); break;
				case Role::SecondDefender: key.add(m_skat_1); break;
			}
				
			if (
				auto it = m_look_up.find(key);
				it != m_look_up.end()
			) {
				return it->second;
			}

			auto [it, succ] = m_look_up.emplace(key, quick_bounds(sit, m_game));
			assert(succ);
			return it->second;
		}

		[[nodiscard]] auto &surely_existing_current_bounds(Situation sit) {
			auto key = sit.hand(Role::Declarer) |
				sit.hand(Role::FirstDefender) |
				sit.hand(Role::SecondDefender);
			switch (sit.active_role()) {
				case Role::Declarer: break;
				case Role::FirstDefender: key.add(m_skat_0); break;
				case Role::SecondDefender: key.add(m_skat_1); break;
			}
			
			auto it = m_look_up.find(key);
			assert(it != m_look_up.end());
			return it->second;
		}
	
	public:
		SituationSolver(
			Situation sit,
			GameType game,
			Card skat_0,
			Card skat_1
		) :
			m_look_up{},
			m_game{game},
			m_skat_0{skat_0},
			m_skat_1{skat_1},
			m_h_dec{sit.hand(Role::Declarer)},
			m_h_fd{sit.hand(Role::FirstDefender)},
			m_h_sd{sit.hand(Role::SecondDefender)}
		{
			[[maybe_unused]] auto strict_bounds_childs = std::pair{
				Bounds{},
				MaybeCard{}
			};
			m_look_up[Cards{}] = strict_bounds_childs;
			auto c = Cards{};
			c.add(skat_0);
			m_look_up[c] = strict_bounds_childs;
			c = Cards{};
			c.add(skat_1);
			m_look_up[c] = strict_bounds_childs;
			assert(m_look_up.size() == 3);
		}


		[[nodiscard]] auto number_of_nodes() const {
			return m_look_up.size();
		}

	private:
		//This is the highly optimized one.
		template<bool is_declarer>
		[[nodiscard]] auto improve_bounds_to_decide_threshold(
			std::pair<Bounds, MaybeCard> bounds_pref,
			Situation sit,
			Score threshold
		) -> std::pair<Bounds, MaybeCard> {
			using namespace stdc::literals;
			
			assert((sit.active_role() == Role::Declarer) == is_declarer);

			auto &[bounds, pref] = bounds_pref;

			assert(!decides_threshold(bounds, threshold));

			auto bound_calculated_over_all_children = is_declarer ? Score{0, 0} : Score{120, 10};

			auto cards_to_consider = get_cards_to_consider(sit, m_game, pref);

			//We catch these via strict bounds.
			assert(cards_to_consider[0]);

			auto maybe_deciding_card = MaybeCard{};

			for (auto mcard : cards_to_consider) {
				if (!mcard) {
					break;
				}

				auto card = *mcard;

				auto child = sit;
				auto additional_score = child.play_card(card, m_game);
				auto threshold_child = required_beyond_to_reach(additional_score, threshold);

				auto bounds_child = bounds_deciding_threshold(child, threshold_child);
				
				auto lower_bound_via_child = bounds_child.lower();
				lower_bound_via_child.add(additional_score);
				
				auto upper_bound_via_child = bounds_child.upper();
				upper_bound_via_child.add(additional_score);

				assert(decides_threshold(
					Bounds{lower_bound_via_child, upper_bound_via_child},
					threshold
				));

				if constexpr (is_declarer) {
					stdc::maximize(bounds.m_lower, lower_bound_via_child);
					stdc::maximize(bound_calculated_over_all_children, upper_bound_via_child);
				} else {
					stdc::minimize(bound_calculated_over_all_children, lower_bound_via_child);
					stdc::minimize(bounds.m_upper, upper_bound_via_child);
				}
				
				if (decides_threshold(bounds, threshold)) {
					maybe_deciding_card = card;
					break;
				}
			}
			
			if (!maybe_deciding_card) {
				if constexpr (is_declarer) {
					//No winning child.
					bounds.update_upper(bound_calculated_over_all_children);
				} else {
					//All childs win.
					bounds.update_lower(bound_calculated_over_all_children);
				}

				// maybe_deciding_card = pref;
			}

			return {bounds, maybe_deciding_card};
		}

	public:
		[[nodiscard]] auto bounds_deciding_threshold(Situation sit, Score threshold) -> Bounds {

			if (!sit.get_maybe_first_trick_card()) {
				//Old path.
				auto bounds_pref = current_bounds(sit);
				if (!decides_threshold(bounds_pref.first, threshold)) {
					if (sit.active_role() == Role::Declarer) {
						bounds_pref = improve_bounds_to_decide_threshold<true>(bounds_pref, sit, threshold);
					} else {
						bounds_pref = improve_bounds_to_decide_threshold<false>(bounds_pref, sit, threshold);
					}
					assert(decides_threshold(bounds_pref.first, threshold));
					
					surely_existing_current_bounds(sit) = bounds_pref;
				}
				return bounds_pref.first;
			}

			//New path.
			auto bounds_pref = quick_bounds(sit, m_game);
			if (!decides_threshold(bounds_pref.first, threshold)) {
				if (sit.active_role() == Role::Declarer) {
					bounds_pref = improve_bounds_to_decide_threshold<true>(bounds_pref, sit, threshold);
				} else {
					bounds_pref = improve_bounds_to_decide_threshold<false>(bounds_pref, sit, threshold);
				}
				assert(decides_threshold(bounds_pref.first, threshold));
			}
			return bounds_pref.first;

		}

		[[nodiscard]] auto still_makes_at_least(Situation sit, Score expected_score) {
			const auto bounds = bounds_deciding_threshold(sit, expected_score);
			if (expected_score <= bounds.lower()) {
				return true;
			}

			assert(bounds.upper() < expected_score);
			return false;
		}

		//Undefined if no card is left.
		//Declarer picks any card that will result in reaching the threshold, if possible.
		//Defender picks any card that will result in missing the threshold, if possible.
		auto maybe_card_for_threshold(Situation sit, Score expected_score)
			-> std::optional<Card>
		{
			auto possible_plays = next_possible_plays(sit, m_game);
			assert(!possible_plays.empty());

			for (auto i = 0; i < 32; ++i) {
				auto card = static_cast<Card>(i);
				
				if (!possible_plays.contains(card)) {
					continue;
				}

				auto child = sit;
				auto trick_score = child.play_card(card, m_game);
				auto expected_score_child = required_beyond_to_reach(trick_score, expected_score);
				auto makes_it_child = still_makes_at_least(child, expected_score_child);

				if (makes_it_child) {
					if (sit.active_role() == Role::Declarer) {
						return card;
					}
				} else {
					if (sit.active_role() != Role::Declarer) {
						assert((expected_score != Score{0, 0}));
						return card;
					}
				}
			}

			return std::nullopt;
		}

		//If everyone plays perfect, this is the score still made from here.
		auto calculate_potential_score_2(Situation sit) -> Score {
			auto goal = Score{0, 0};
			while (still_makes_at_least(sit, goal)) {
				goal = Score{static_cast<uint8_t>(goal.points() + 1), 0};
			}
			assert(0 < goal.points());
			goal = Score{static_cast<uint8_t>(goal.points() - 1), 0};
			assert(goal.points() <= 120);
			return goal;
		}

		auto calculate_potential_score_3(Situation sit) -> Score {
			auto goal = Score{121, 0};
			while (!still_makes_at_least(sit, goal)) {
				assert(0 < goal.points());
				goal = Score{static_cast<uint8_t>(goal.points() - 1), 0};
			
			}
			assert(goal.points() <= 120);
			return goal;
		}

		//Undefined if no card is left.
		auto pick_best_card_in_situation(Situation sit) -> std::pair<Card, Score> {
			auto final_additional_score_to_reach = calculate_potential_score_2(sit);

			if (sit.active_role() == Role::Declarer) {
				//Pick a card that forces at least this value.
				return {stdc::surely(maybe_card_for_threshold(sit, final_additional_score_to_reach)), final_additional_score_to_reach};
			}
			
			//TODO: How to handle Schwarz here?
			auto final_additional_score_to_prevent = Score{
				static_cast<uint8_t>(final_additional_score_to_reach.points() + 1),
				final_additional_score_to_reach.tricks()
			};

			//Pick a card that forces at most this value (so less than this value + 1).
			return {stdc::surely(maybe_card_for_threshold(sit, final_additional_score_to_prevent)), final_additional_score_to_reach};
		}

		//How many points will still follow for the declarer due to finishing tricks?
		//Not counting tricks already won, or points gedrückt.
		auto score_for_possible_plays(Situation sit) {
			using namespace stdc::literals;
			
			auto inv = Score{121, 0};

			auto result = std::array<Score, 32>{
				inv, inv, inv, inv, 
				inv, inv, inv, inv, 
				inv, inv, inv, inv, 
				inv, inv, inv, inv, 
				inv, inv, inv, inv, 
				inv, inv, inv, inv, 
				inv, inv, inv, inv, 
				inv, inv, inv, inv
			};
			auto possible_plays = next_possible_plays(sit, m_game);
			assert(!possible_plays.empty());

			for (auto i = 0_z; i < 32; ++i) {
				auto card = static_cast<Card>(i);
				
				if (!possible_plays.contains(card)) {
					continue;
				}

				auto child = sit;
				auto points_to_get_to_child = child.play_card(card, m_game);
				auto points_as_child = calculate_potential_score_3(child);
				auto future_points_for_tricks_when_choosing_child = points_as_child;
				future_points_for_tricks_when_choosing_child.add(points_to_get_to_child);
				result[i] = future_points_for_tricks_when_choosing_child;
			}

			return result;
		}



	};


	// inline auto score_for_possible_plays_separate(Situation sit, GameType game, Card skat_0, Card skat_1) {
	// 	using namespace stdc::literals;
		
	// 	auto result = std::array<uint8_t, 32>{
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121, 
	// 		121, 121, 121, 121
	// 	};


	// 	auto possible_plays = next_possible_plays(sit, game);
	// 	assert(!possible_plays.empty());

	// 	auto nodes = std::vector<double>{};
	// 	nodes.reserve(possible_plays.size());

	// 	for (auto i = 0_z; i < 32; ++i) {
	// 		auto card = static_cast<Card>(i);
			
	// 		if (!possible_plays.contains(card)) {
	// 			continue;
	// 		}

	// 		auto solver = SituationSolver{sit, game, skat_0, skat_1};

	// 		auto child = sit;
	// 		auto points_to_get_to_child = child.play_card(card, game);
	// 		auto points_as_child = solver.calculate_potential_score_3(child);
	// 		result[i] = points_as_child + points_to_get_to_child;
	// 		nodes.push_back(static_cast<double>(solver.number_of_nodes()) / 1000.);
	// 	}

	// 	return std::pair{result, nodes};
	// }


} // namespace muskat
