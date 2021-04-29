#pragma once

#include "abstract_player.h"

#include <stdc/literals.h>

#include <string>
#include <random>

namespace muskat {
	class RandomPlayer : public AbstractPlayer {
	private:
		std::string m_name;

		void say(const auto &string) {
			std::cout << '[' << m_name << "]: " << string << '\n';
		}

		Cards m_hand;
		size_t m_number_of_cards_already_played;
		GameType m_game;
		std::optional<TrickAndGameType> m_maybe_type{};
		std::mt19937 m_rng;

	public:
		void inform_about_role(Role role) final {
			using namespace stdc::literals;
			say("My role is "s + to_string(role) + ".");
		}

		virtual void inform_about_first_position(Position position) final {
			using namespace stdc::literals;
			say("My first position is "s + to_string(position) + ".");
		}

		virtual void inform_about_game(GameType game) final {
			using namespace stdc::literals;
			say("We play "s + to_string(game) + ".");
			m_game = game;
		}

		virtual void inform_about_deal(Cards cards) final {
			using namespace stdc::literals;
			say("I was dealt the following hand: "s + to_string(cards) + ".");
			m_hand = cards;
			m_number_of_cards_already_played = 0;
		}

		virtual void inform_about_skat(Cards cards) final {
			using namespace stdc::literals;
			say("In the skat I found "s + to_string(cards) + ".");
		}

		virtual void inform_about_move(Card card) final {
			using namespace stdc::literals;
			say("Card "s + to_string(card) + " was played.");

			assert(!m_hand.contains(card));
			
			if (m_number_of_cards_already_played % 3 == 0) {
				m_maybe_type = TrickAndGameType{card, m_game};
			}
			
			++m_number_of_cards_already_played;

			if (m_number_of_cards_already_played % 3 == 0) {
				m_maybe_type = std::nullopt;
			}
		}

		virtual auto request_move() -> Card final {
			auto card = random_card_from(get_legal_cards(m_hand, m_maybe_type), m_rng);
			m_hand.remove(card);
			return card;
		}

		RandomPlayer(
			std::string name,
			std::mt19937 &&rng
		) :
			m_name{std::move(name)},
			m_rng{std::move(rng)}
		{}
	};
} // namespace muskat
