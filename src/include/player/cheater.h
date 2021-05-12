#pragma once

#include "abstract_player.h"
#include "situation_solver.h"

#include <stdc/literals.h>
#include <stdc/WATCH.h>

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
		Points m_points_declarer;

	public:
		void cheat(const Situation &situation) override {
			m_current_situation = situation;

			auto skat = m_current_situation.cellar();
			assert(skat.size() == 2);
			auto skat_0 = skat.remove_next();
			auto skat_1 = skat.remove_next();

			m_solver = SituationSolver{m_current_situation, m_game, skat_0, skat_1}; //Let's free some memory.

			say("Secretly peeking at hidden cards to cheat later.");
			WATCH("decide").reset();
			WATCH("decide").start();
			auto [card, worst_case_score_from_here] = m_solver.pick_best_card(m_current_situation);
			WATCH("decide").stop();
			say(
				"This already tells me that for perfect play with no hidden information, the final score would be " +
				std::to_string(static_cast<size_t>(worst_case_score_from_here)) +
				'.'
			);
			say("Thinking this through took " + std::to_string(WATCH("decide").elapsed<std::chrono::milliseconds>()) + " ms.");
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
			say("Cheating to find the perfect move …");
			WATCH("decide").reset();
			WATCH("decide").start();
			auto [card, worst_case_score_from_here] = m_solver.pick_best_card(m_current_situation);
			WATCH("decide").stop();
			auto end_of_sentence = m_role == Role::Declarer ? std::string{" or more."} : std::string{" or less."};
			say(
				"Decided on " +
				to_string(card) +
				" to force score " +
				std::to_string(static_cast<size_t>(m_points_declarer + worst_case_score_from_here)) +
				end_of_sentence
			);
			say("Cheating took " + std::to_string(WATCH("decide").elapsed<std::chrono::milliseconds>()) + " ms.");
			return card;
		}

		Cheater(
			std::string name,
			std::mt19937 &&rng
		) :
			m_name{std::move(name)},
			m_rng{std::move(rng)},
			m_solver{Situation{Role::Declarer}, GameType::Eichel, Card::E7, Card::E8} //TODO
		{}
	};
} // namespace muskat
