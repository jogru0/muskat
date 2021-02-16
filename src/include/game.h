#pragma once

#include "situation.h"

#include "abstract_player.h"

namespace muskat {

	enum class Position {
		Vorderhand, Mittelhand, Hinterhand
	};

	[[nodiscard]] inline void play_one_game(
		AbstractPlayer &geber,
		AbstractPlayer &hoerer,
		AbstractPlayer &sager,
		const Deck &deck
	) {
		using namespace stdc::literals;

		geber.inform_about_first_position(Position::Hinterhand);
		hoerer.inform_about_first_position(Position::Vorderhand);
		sager.inform_about_first_position(Position::Mittelhand);

		auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);

		geber.inform_about_deal(hand_geber);
		hoerer.inform_about_deal(hand_hoerer);
		sager.inform_about_deal(hand_sager);

		//For now no Reizen, just assign the roles.
		auto &declarer = geber;
		auto &hand_declarer = hand_geber;
		auto &first_defender = hoerer;
		auto &hand_first_defender = hand_hoerer;
		auto &second_defener = sager;
		auto &hand_second_defener = hand_sager;
		
		auto role_of_winner_last_tick = Role::FirstDefender; //Hören == Kommen
		
		auto role_to_player_and_hand = [&](auto role) -> std::tuple<AbstractPlayer &, Cards &> {
			switch (role) {
				case Role::Declarer : return {declarer, hand_declarer};
				case Role::FirstDefender : return {first_defender, hand_first_defender};
				case Role::SecondDefender : return {second_defener, hand_second_defener};
			}
		}
		
		declarer.inform_about_role(Role::Declarer);
		first_defender.inform_about_role(Role::FirstDefender);
		second_defener.inform_about_role(Role::SecondDefender);

		
		//Also no Handspiel.
		hand_declarer |= skat;
		declarer.inform_about_skat(skat);

		//Also no Drücken or Spielansagen.
		hand_declarer &= ~skat;
		auto game_type = GameType::Herz;
		
		geber.inform_about_game(game_type);
		hoerer.inform_about_game(game_type);
		sager.inform_about_game(game_type);

		auto points_declarer = to_points(skat);
		auto points_defender = 0_z;
		
		for (auto number_of_trick = 1_z; number_of_trick != 11; ++number_of_trick) {
			auto [player, hand] = role_to_player_and_hand(role_of_winner_last_tick);
			auto first_card = player.request_move();
			//CHECK IF HE HAS THIS CARD.
			auto trick_type = get_trick_type(first_card, game_type);


		}


	}
} // namespace muskat
