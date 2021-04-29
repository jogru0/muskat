#pragma once

#include "abstract_player.h"
#include "world_simulation.h"

#include <stdc/literals.h>
#include <stdc/WATCH.h>

#include <string>
#include <random>

namespace muskat {
	class UniformSampler : public AbstractPlayer {
	private:
		std::string m_name;

		void say(const auto &string) {
			std::cout << '[' << m_name << "]: " << string << '\n';
		}

		PossibleWorlds m_current_information;
		GameType m_game;
		std::mt19937 m_rng;
		Points m_points_declarer;


		//Hopefully there when we get informed about the game …
		Role m_role;
		Cards m_hand;
		Cards m_skat; //Only exception: This is not there if we are not the declarer.
		Position m_my_first_position;

	public:

		void inform_about_first_position(Position position) final {
			using namespace stdc::literals;
			say("My first position is "s + to_string(position) + ".");
			m_my_first_position = position;
		}


		void inform_about_role(Role role) final {
			using namespace stdc::literals;
			say("My role is "s + to_string(role) + ".");
			m_role = role;
		}


		virtual void inform_about_game(GameType game) final {
			using namespace stdc::literals;
			say("We play "s + to_string(game) + ".");
			m_points_declarer = 0;
			say("For debugging: I think my role is " + to_string(m_role) + ".");
			say("For debugging: I think my position is " + to_string(m_my_first_position) + ".");
			say("For debugging: I think my hand is " + to_string(m_hand) + ".");
			if (m_role == Role::Declarer) {
				say("For debugging: I think the skat is " + to_string(m_skat) + ".");
			}

			auto known_about_unknown_dec_fdef_sdef_skat = std::array{
				muskat::KnownUnknownInSet{
					10, {true, true, true, true, true}
				},
				muskat::KnownUnknownInSet{
					10, {true, true, true, true, true}
				},
				muskat::KnownUnknownInSet{
					10, {true, true, true, true, true}
				},
				muskat::KnownUnknownInSet{
					2, {true, true, true, true, true}
				}
			};

			known_about_unknown_dec_fdef_sdef_skat[static_cast<size_t>(m_role)].number = 0;

			if (m_role == Role::Declarer) {
				known_about_unknown_dec_fdef_sdef_skat[3].number = 0;
			}

			auto known_cards_dec_fdef_sdef_skat = std::array{
				muskat::Cards{},
				muskat::Cards{},
				muskat::Cards{},
				muskat::Cards{},
			};

			known_cards_dec_fdef_sdef_skat[static_cast<size_t>(m_role)] = m_hand;

			if (m_role == Role::Declarer) {
				known_cards_dec_fdef_sdef_skat[3] = m_skat;
			}

			auto role_position_match = std::pair{m_role, m_my_first_position};
			while (role_position_match.second != Position::Vorhand) {
				role_position_match.first = next(role_position_match.first);
				role_position_match.second = next(role_position_match.second);
			}
			auto active_role = role_position_match.first;
			say("For debugging: I think the first active role is " + to_string(active_role) + ".");
			
			auto known_cards = m_hand;
			if (m_role == Role::Declarer) {
				known_cards |= m_skat;
			}
			say("For debugging: I think my known cards are " + to_string(known_cards) + ".");
			auto unknown_cards = ~known_cards;

			m_current_information = muskat::PossibleWorlds{
				known_about_unknown_dec_fdef_sdef_skat,
				known_cards_dec_fdef_sdef_skat,
				unknown_cards,
				game,
				active_role,
				muskat::MaybeCard{},
				muskat::MaybeCard{}
			};
		}

		virtual void inform_about_deal(Cards cards) final {
			using namespace stdc::literals;
			say("I was dealt the following hand: "s + to_string(cards) + ".");
			m_hand = cards;
		}

		virtual void inform_about_skat(Cards cards) final {
			using namespace stdc::literals;
			say("In the skat I found "s + to_string(cards) + ".");
			m_skat = cards;
		}

		virtual void inform_about_move(Card card) final {
			using namespace stdc::literals;
			say("Card "s + to_string(card) + " was played.");
			m_points_declarer += m_current_information.play_card(card, m_game);
		}

		virtual auto request_move() -> Card final {
			assert(m_current_situation.active_role() == m_role);
			assert(!is_at_game_end(m_current_situation));
			WATCH("decide").reset();
			WATCH("decide").start();
			auto card = m_solver.pick_best_card(m_current_situation);
			WATCH("decide").stop();
			std::cout << "I thought for " << WATCH("decide").elapsed<std::chrono::milliseconds>() << " ms.\n";
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
