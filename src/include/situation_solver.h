#pragma once

#include "situation.h"

#include <stdc/mathematics.h>

#include <robin_hood/robin_hood.h>

namespace muskat {


	class Bounds {
	private:
		Points m_lower;
		Points m_upper;
	public:
		explicit constexpr Bounds(Points lower, Points upper) :
			m_lower{lower}, m_upper{upper}
		{
			assert(m_lower <= m_upper);
		}
		
		explicit constexpr Bounds() : Bounds{0, 0} {}

		[[nodiscard]] constexpr auto lower() const{ return m_lower; }
		[[nodiscard]] constexpr auto upper() const{ return m_upper; }

		constexpr void update_lower(Points new_value) {
			assert (m_lower < new_value);
			m_lower = new_value;
			assert(m_lower <= m_upper);
		}
		constexpr void update_upper(Points new_value) {
			assert(new_value < m_upper);
			m_upper = new_value;
			assert(m_lower <= m_upper);	
		}
	};

	[[nodiscard]] inline constexpr auto operator==(Bounds left, Bounds right) {
		return left.lower() == right.lower() && left.upper() == right.upper();
	}

	[[nodiscard]] inline constexpr auto decides_threshold(Bounds bounds, Points threshold) {
		return threshold <= bounds.lower() || bounds.upper() < threshold;
	}

	[[nodiscard]] inline auto quick_bounds(Situation sit, GameType game) {
		constexpr auto max_points = Points{120};
		auto upper = static_cast<Points>(max_points - to_points(sit.cellar(), game));
		return Bounds{0, upper};
	}

	class SituationSolver {
	private:
		using LookUpTable = robin_hood::unordered_flat_map<Situation, Bounds>;

		LookUpTable m_look_up;
		GameType m_game;

		[[nodiscard]] auto &current_bounds(Situation sit) {
			if (
				auto it = m_look_up.find(sit);
				it != m_look_up.end()
			) {
				return it->second;
			}

			auto [it, succ] = m_look_up.emplace(sit, quick_bounds(sit, m_game));
			assert(succ);
			return it->second;
		}
	
	public:
		SituationSolver(GameType game) : m_look_up{}, m_game{game} {
			[[maybe_unused]] auto strict_bounds_childs = Bounds{};
			m_look_up[Situation{Role::Declarer}] = strict_bounds_childs;
			m_look_up[Situation{Role::FirstDefender}] = strict_bounds_childs;
			m_look_up[Situation{Role::SecondDefender}] = strict_bounds_childs;
			assert(m_look_up.size() == 3);
		}


		[[nodiscard]] auto number_of_nodes() const {
			return m_look_up.size();
		}

	private:
		template<bool is_declarer>
		[[nodiscard]] auto improve_bounds_to_decide_threshold(Bounds bounds, Situation sit, Points threshold) {
			assert((sit.active_role() == Role::Declarer) == is_declarer);

			assert(!decides_threshold(bounds, threshold));

			auto other_bound = is_declarer ? Points{} : Points{120};

			auto possible_plays = next_possible_plays(sit, m_game);
			//We catch these via strict bounds.
			assert(!possible_plays.empty());

			for (auto i = 0; i < 32; ++i) {
				auto card = static_cast<Card>(i);
				
				if (!possible_plays.contains(card)) {
					continue;
				}

				auto child = sit;
				auto points = child.play_card(card, m_game);
				auto threshold_child = threshold <= points ? Points{} : static_cast<Points>(threshold - points);

				auto bounds_child = bounds_deciding_threshold(child, threshold_child);

				if constexpr (is_declarer) {
					stdc::maximize(other_bound, static_cast<Points>(bounds_child.upper() + points));
					
					auto new_lower = static_cast<Points>(bounds_child.lower() + points);
					if (bounds.lower() < new_lower) {
						bounds.update_lower(new_lower);
						if (threshold <= new_lower) {
							return bounds;
						}
					}
				} else {
					stdc::minimize(other_bound, static_cast<Points>(bounds_child.lower() + points));
					
					auto new_upper = static_cast<Points>(bounds_child.upper() + points);
					if (new_upper < bounds.upper()) {
						bounds.update_upper(new_upper);
						if (new_upper < threshold) {
							return bounds;
						}
					}
				}
			}

			if constexpr (is_declarer) {
				//No winning child.
				bounds.update_upper(other_bound);
				return bounds;
			} else {
				//All childs win.
				bounds.update_lower(other_bound);
				return bounds;
			}
		}

	public:
		[[nodiscard]] auto bounds_deciding_threshold(Situation sit, Points threshold) -> Bounds {
			auto bounds = current_bounds(sit);
			if (!decides_threshold(bounds, threshold)) {
				switch (sit.active_role() == Role::Declarer) {
					case true: bounds = improve_bounds_to_decide_threshold<true>(bounds, sit, threshold); break;
					case false: bounds = improve_bounds_to_decide_threshold<false>(bounds, sit, threshold);
				}
				assert(decides_threshold(bounds, threshold));
				current_bounds(sit) = bounds;
			}
			return bounds;
		}

		[[nodiscard]] auto still_makes_at_least(Situation sit, Points expected_points) {
			const auto bounds = bounds_deciding_threshold(sit, expected_points);
			if (expected_points <= bounds.lower()) {
				return true;
			}

			assert(bounds.upper() < expected_points);
			return false;
		}

		//Undefined if no card is left.
		//Declarer picks any card that will result in reaching the threshold, if possible.
		//Defender picks any card that will result in missing the threshold, if possible.
		auto maybe_card_for_threshold(Situation sit, Points expected_points)
			-> std::optional<Card>
		{
			// auto &[lower, upper] = current_bounds(sit);
			// if (expected_points <= lower) {
			// 	return true;
			// }
			// if (upper < expected_points) {
			// 	return false;
			// }

			auto possible_plays = next_possible_plays(sit, m_game);
			assert(!possible_plays.empty());

			for (auto i = 0; i < 32; ++i) {
				auto card = static_cast<Card>(i);
				
				if (!possible_plays.contains(card)) {
					continue;
				}

				auto child = sit;
				auto points = child.play_card(card, m_game);
				auto expected_points_child = expected_points <= points ? Points{} : static_cast<Points>(expected_points - points);

				auto makes_it_child = still_makes_at_least(child, expected_points_child);

				if (makes_it_child) {
					if (sit.active_role() == Role::Declarer) {
						// lower = expected_points;
						return card;
					}
				} else {
					if (sit.active_role() != Role::Declarer) {
						assert(expected_points != 0);
						// upper = expected_points - 1;
						return card;
					}
				}
			}

			if (sit.active_role() == Role::Declarer) {
				//No winning child.
				// upper = expected_points - 1;
				// return false;
			} else {
				//All childs win.
				// lower = expected_points;
				// return false;
			}
			
			return std::nullopt;
		}


	auto calculate_potential_score(Situation sit) -> Points {
		if (sit.active_role() == Role::Declarer) {
			auto goal = Points{};
			while (still_makes_at_least(sit, goal)) {
				++goal;
			}
			assert(0 < goal);
			--goal;
			assert(goal <= 120);
			return goal;
		}

		auto goal = Points{121};
		while (!still_makes_at_least(sit, goal)) {
			assert(0 < goal);
			--goal;
		
		}
		assert(goal <= 120);
		return goal;
	}

	//Undefined if no card is left.
	auto pick_best_card(Situation sit) -> Card {
		auto threshold = calculate_potential_score(sit);

		if (sit.active_role() == Role::Declarer) {
			std::cout << "Picking card to reach at least " << threshold << ".\n";
			return stdc::surely(maybe_card_for_threshold(sit, threshold));
		}

		std::cout << "Picking card to not let him reach " << threshold + 1 << ".\n";
		return stdc::surely(maybe_card_for_threshold(sit, threshold + 1));
	}
};

} // namespace muskat