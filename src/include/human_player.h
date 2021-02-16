#pragma once

#include "abstract_player.h"

#include <stdc/literals.h>

#include <string>

namespace muskat {
	class HumanPlayer : public AbstractPlayer {
	private:
		std::string m_name;

		void say(const auto &string) {
			std::cout << '[' << m_name << "]: " << string << '\n';
		}

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
		}

		virtual void inform_about_deal(Cards cards) final {
			using namespace stdc::literals;
			say("I was dealt the following hand: "s + to_string(cards) + ".");
		}

		virtual void inform_about_skat(Cards cards) final {
			using namespace stdc::literals;
			say("In the skat i found "s + to_string(cards) + ".");
		}

		virtual void inform_about_move(Card card) final {
			using namespace stdc::literals;
			say("Card "s + to_string(card) + " was played.");
		}

		virtual auto request_move() -> Card final {
			using namespace stdc::literals;
			say("Choose next card to play:"s);
			Card card;
			std::cin >> card;
			return card;
		}

		HumanPlayer(std::string name) : m_name(std::move(name)) {}
	};
} // namespace muskat
