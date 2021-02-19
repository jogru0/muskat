#pragma once

#include "situation.h"

#include "abstract_player.h"

namespace muskat {

	inline void play_card_from_hand(
		Cards &hand,
		Card card,
		std::optional<TrickAndGameType> maybe_trick_game_type
	) {
		auto legal_cards = maybe_trick_game_type
			? legal_response_cards(hand, *maybe_trick_game_type)
			: hand;
		
		if (!legal_cards[card]) {
			//TODO: Correctly handle this.
			assert(false);
		}

		auto card_in_hand = hand.get_ref(card);
		assert(card_in_hand);
		card_in_hand = false;
	}


	[[nodiscard]] inline auto play_one_game(
		AbstractPlayer &geber,
		AbstractPlayer &hoerer,
		AbstractPlayer &sager,
		const Deck &deck
	) {
		using namespace stdc::literals;

		geber.inform_about_first_position(Position::Hinterhand);
		hoerer.inform_about_first_position(Position::Vorhand);
		sager.inform_about_first_position(Position::Mittelhand);
		auto role_of_winner_last_tick = Role::FirstDefender; //Hören == Kommen

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
		
		auto role_to_player_and_hand = [&](auto role) -> std::tuple<AbstractPlayer &, Cards &> {
			switch (role) {
				case Role::Declarer : return {declarer, hand_declarer};
				case Role::FirstDefender : return {first_defender, hand_first_defender};
				case Role::SecondDefender : return {second_defener, hand_second_defener};
			}
		};
		
		declarer.inform_about_role(Role::Declarer);
		first_defender.inform_about_role(Role::FirstDefender);
		second_defener.inform_about_role(Role::SecondDefender);

		
		//Also no Handspiel.
		hand_declarer |= skat;
		declarer.inform_about_skat(skat);

		//Also no choice for Drücken or Spielansagen.
		auto gedrueckt = skat;
		hand_declarer &= ~gedrueckt;
		auto game_type = GameType::Herz;
		
		geber.inform_about_game(game_type);
		hoerer.inform_about_game(game_type);
		sager.inform_about_game(game_type);

		auto points_declarer = to_points(gedrueckt);
		auto points_defender = 0_z;

		auto situation = Situation{
			hand_declarer,
			hand_first_defender,
			hand_second_defener,
			gedrueckt,
			role_of_winner_last_tick
		};

		geber.cheat(situation);
		hoerer.cheat(situation);
		sager.cheat(situation);
		

		auto process_one_move = [&](
			Role role,
			std::optional<TrickAndGameType> maybe_type = std::nullopt
		) {
			auto [player, hand] = role_to_player_and_hand(role);

			auto card_to_play = player.request_move();
			play_card_from_hand(hand, card_to_play, maybe_type);

			geber.inform_about_move(card_to_play);
			hoerer.inform_about_move(card_to_play);
			sager.inform_about_move(card_to_play);

			return card_to_play;
		};
		
		for (auto number_of_trick = 1_z; number_of_trick != 11; ++number_of_trick) {
			auto role_vorhand = role_of_winner_last_tick;
			auto role_mittelhand = next(role_vorhand);
			auto role_hinterhand = next(role_mittelhand);
			
			auto first_card = process_one_move(role_vorhand);
			auto trick_game_type = TrickAndGameType{first_card, game_type};
			auto second_card = process_one_move(role_mittelhand, trick_game_type);
			auto third_card = process_one_move(role_hinterhand, trick_game_type);

			auto trick = Trick{first_card, second_card, third_card};

			switch (trick_winner_position(trick, trick_game_type)) {
				case Position::Vorhand: role_of_winner_last_tick = role_vorhand; break;
				case Position::Mittelhand: role_of_winner_last_tick = role_mittelhand; break;
				case Position::Hinterhand: role_of_winner_last_tick = role_hinterhand;
			}

			auto trick_points = to_points(trick);
			if (role_of_winner_last_tick == Role::Declarer) {
				points_declarer += trick_points;
			} else {
				points_defender += trick_points;
			}
		}

		assert(points_defender + points_declarer == 120_z);
		assert(hand_geber.empty());
		assert(hand_hoerer.empty());
		assert(hand_sager.empty());

		return points_declarer;
	}
} // namespace muskat
