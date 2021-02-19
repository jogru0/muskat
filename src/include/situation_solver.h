#pragma once

#include "situation.h"

#include <unordered_map>

namespace muskat {

	class Bounds {
	private:
		uint_fast16_t m_lower;
		uint_fast16_t m_upper;
	public:
		explicit constexpr Bounds(uint_fast16_t lower, uint_fast16_t upper) :
			m_lower{lower}, m_upper{upper}
		{
			assert(m_lower <= m_upper);
		}
		
		explicit constexpr Bounds() : Bounds{0, 0} {}

		[[nodiscard]] constexpr auto lower() const{ return m_lower; }
		[[nodiscard]] constexpr auto upper() const{ return m_upper; }

		constexpr void update_lower(uint_fast16_t new_value) {
			assert (m_lower < new_value);
			m_lower = new_value;
			assert(m_lower <= m_upper);
		}
		constexpr void update_upper(uint_fast16_t new_value) {
			assert(new_value < m_upper);
			m_upper = new_value;
			assert(m_lower <= m_upper);	
		}
	};

	[[nodiscard]] inline constexpr auto operator==(Bounds left, Bounds right) {
		return left.lower() == right.lower() && left.upper() == right.upper();
	}

	[[nodiscard]] inline auto quick_bounds(Situation sit) {
		return Bounds{0, 120 - to_points(sit.cellar())};
	}

	class SituationSolver {
	private:
		using LookUpTable = std::unordered_map<Situation, Bounds>;

		LookUpTable m_look_up;
		GameType m_game;

		[[nodiscard]] auto &current_bounds(Situation sit) {
			if (
				auto it = m_look_up.find(sit);
				it != m_look_up.end()
			) {
				return it->second;
			}

			auto [it, succ] = m_look_up.emplace(sit, quick_bounds(sit));
			assert(succ);
			return it->second;
		}
	
	public:
		SituationSolver(GameType game) : m_look_up{}, m_game{game} {
			[[maybe_unused]] auto expected_bounds = Bounds{};
			[[maybe_unused]] auto b_d = current_bounds(Situation{Role::Declarer});
			[[maybe_unused]] auto b_fd = current_bounds(Situation{Role::FirstDefender});
			[[maybe_unused]] auto b_sd = current_bounds(Situation{Role::SecondDefender});
			assert(expected_bounds == b_d);
			assert(expected_bounds == b_fd);
			assert(expected_bounds == b_sd);
			assert(m_look_up.size() == 3);
		}

		auto still_makes_at_least(Situation sit, uint_fast16_t expected_points) {
			auto &bounds = current_bounds(sit);
			if (expected_points <= bounds.lower()) {
				return true;
			}
			if (bounds.upper() < expected_points) {
				return false;
			}

			auto possible_plays = next_possible_plays(sit, m_game);
			assert(!possible_plays.empty());

			//TODO: Not sharp. What if the child makes it because we found out it can make more before we were even able to stop?
			for (auto i = 0; i < 32; ++i) {
				auto card = static_cast<Card>(i);
				
				if (!possible_plays[card]) {
					continue;
				}

				auto child = sit;
				auto points = child.play_card(card, m_game);
				auto expected_points_child = expected_points <= points ? 0 : expected_points - points;

				auto makes_it_child = still_makes_at_least(child, expected_points_child);

				if (makes_it_child) {
					if (sit.active_role() == Role::Declarer) {
						bounds.update_lower(expected_points);
						return true;
					}
				} else {
					if (sit.active_role() != Role::Declarer) {
						assert(expected_points != 0);
						bounds.update_upper(expected_points - 1);
						return false;
					}
				}
			}

			if (sit.active_role() == Role::Declarer) {
				//No winning child.
				bounds.update_upper(expected_points - 1);
				return false;
			} else {
				//All childs win.
				bounds.update_lower(expected_points);
				return true;
			}
		}

		//Undefined if no card is left.
		//Declarer picks any card that will result in reaching the threshold, if possible.
		//Defender picks any card that will result in missing the threshold, if possible.
		auto maybe_card_for_threshold(Situation sit, uint_fast16_t expected_points)
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
				
				if (!possible_plays[card]) {
					continue;
				}

				auto child = sit;
				auto points = child.play_card(card, m_game);
				auto expected_points_child = expected_points <= points ? 0 : expected_points - points;

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

	//Undefined if no card is left.
	auto pick_best_card(Situation sit) -> Card {
		if (sit.active_role() == Role::Declarer) {
			auto goal = uint_fast16_t{};
			while (still_makes_at_least(sit, goal)) {
				++goal;
			}
			assert(0 < goal);
			--goal;
			assert(goal <= 120);
			std::cout << "Picking card to reach at least " << goal << ".\n";
			return stdc::surely(maybe_card_for_threshold(sit, goal));
		}

		auto goal = uint_fast16_t{121};
		while (!still_makes_at_least(sit, goal)) {
			assert(0 < goal);
			--goal;
		
		}
		assert(goal <= 120);
		++goal;
		std::cout << "Picking card to not let him reach " << goal << ".\n";
		return stdc::surely(maybe_card_for_threshold(sit, goal));
	}
};

} // namespace muskat
