#pragma once

#include "abstract_player.h"
#include "world_simulation.h"
#include "concurrent_monte_carlo.h"

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

		std::optional<PossibleWorlds> m_current_information;
		std::mt19937 m_rng;
		Score m_score_declarer_without_skat;


		//Hopefully there when we get informed about the game …
		Role m_role;
		Cards m_hand;
		std::optional<Cards> m_skat; //Only exception: This is not there if we are not the declarer.
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

		//Because this is the last call, this is where all the magic happens …
		//TODO: How to architect this?
		virtual void inform_about_game(GameType game) final {
			using namespace stdc::literals;
			say("We play "s + to_string(game) + ".");
			m_score_declarer_without_skat = Score{0, 0};
			say("For debugging: I think my role is " + to_string(m_role) + ".");
			say("For debugging: I think my position is " + to_string(m_my_first_position) + ".");
			say("For debugging: I think my hand is " + to_string(m_hand) + ".");
			assert(m_skat.has_value() == (m_role == Role::Declarer));
			if (m_role == Role::Declarer) {
				say("For debugging: I think the skat is " + to_string(stdc::surely(m_skat)) + ".");
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
				known_cards_dec_fdef_sdef_skat[3] = stdc::surely(m_skat);
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
				known_cards |= stdc::surely(m_skat);
			}
			say("For debugging: I think my known cards are " + to_string(known_cards) + ".");
			
			m_current_information = muskat::PossibleWorlds{
				m_hand,
				m_role,
				m_skat,
				game,
				active_role
			};

			assert(known_about_unknown_dec_fdef_sdef_skat == m_current_information->known_about_unknown_dec_fdef_sdef_skat);
			assert(known_cards_dec_fdef_sdef_skat == m_current_information->known_cards_dec_fdef_sdef_skat);
			assert(~known_cards == m_current_information->unknown_cards);
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
			auto additional_score = stdc::surely(m_current_information).play_card(card);
			m_score_declarer_without_skat.add(additional_score);
		}

		virtual auto request_move() -> Card final {
			assert(m_current_information->active_role == m_role);
			assert(!m_current_information->is_at_game_end());
			say("Deciding my next move …\n");

			//TODO: Not done yet.
			auto contract = Contract{stdc::surely(m_current_information).game, false, false, false, false};
			auto bidding_value = 18;

			auto card = pick_best_card(
				stdc::surely(m_current_information),
				m_score_declarer_without_skat,
				200,
				contract,
				bidding_value
			);
			return card;
		}

		UniformSampler(
			std::string name,
			std::mt19937 &&rng
		) :
			m_name{std::move(name)},
			m_rng{std::move(rng)},
			m_score_declarer_without_skat{0, 0}
		{}
	};
} // namespace muskat
