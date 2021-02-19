#pragma once

#include "abstract_player.h"
#include "situation_solver.h"

#include <stdc/literals.h>

#include <string>
#include <random>

namespace muskat {
	class Cheater : public AbstractPlayer {
	private:
		std::string m_name;

		void say(const auto &string) {
			std::cout << '[' << m_name << "]: " << string << '\n';
		}

		Role m_role;
		Situation m_current_situation;
		GameType m_game;
		std::mt19937 m_rng;
		SituationSolver m_solver;
		uint_fast16_t m_points_declarer;

	public:
		void cheat(const Situation &situation) override {
			m_current_situation = situation;
			m_solver = SituationSolver{m_game}; //Let's free some memory.
		}

		void inform_about_first_position(Position position) final {
			using namespace stdc::literals;
			say("My first position is "s + to_string(position) + ".");
		}


		void inform_about_role(Role role) final {
			using namespace stdc::literals;
			say("My role is "s + to_string(role) + ".");
			m_role = role;
		}


		virtual void inform_about_game(GameType game) final {
			using namespace stdc::literals;
			say("We play "s + to_string(game) + ".");
			m_game = game;
			m_points_declarer = 0;
		}

		virtual void inform_about_deal(Cards cards) final {
			using namespace stdc::literals;
			say("I was dealt the following hand: "s + to_string(cards) + ".");
		}

		virtual void inform_about_skat(Cards cards) final {
			using namespace stdc::literals;
			say("In the skat I found "s + to_string(cards) + ".");
		}

		virtual void inform_about_move(Card card) final {
			using namespace stdc::literals;
			say("Card "s + to_string(card) + " was played.");
			m_points_declarer += m_current_situation.play_card(card, m_game);
		}

		virtual auto request_move() -> Card final {
			assert(m_current_situation.active_role() == m_role);
			assert(!is_at_game_end(m_current_situation));
			auto card = m_solver.pick_best_card(m_current_situation);
			return card;
		}

		Cheater(
			std::string name,
			std::mt19937 &&rng
		) :
			m_name{std::move(name)},
			m_rng{std::move(rng)},
			m_solver{GameType::Eichel} //TODO
		{}
	};
} // namespace muskat
