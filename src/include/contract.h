#pragma once

#include "game.h"


namespace muskat {

[[nodiscard]] auto inline get_spitzen(Cards hand_and_skat, GameType game) -> int {
	if (game == GameType::Null) {
		return 1000;
	}

	assert(hand_and_skat.size() == 12);

	//TODO: We know it's 11 (or 4), vector is overkill.
	auto trump = to_vector(trump_cards(game));
	assert(trump.size() == (game == GameType::Grand ? 4 : 11));

	auto trick_and_game_type = TrickAndGameType{TrickType::Trump, game};

	std::sort(RANGE(trump), [&](auto l, auto r) {
		auto l_power = to_power(l, trick_and_game_type);
		auto r_power = to_power(r, trick_and_game_type);
		assert(l_power != r_power);
		return l_power <= r_power;
	});

	auto highest_trump = trump.back();
	trump.pop_back();
	assert(highest_trump == Card::EU);
	auto is_mit = hand_and_skat.contains(highest_trump);

	auto spitzen = 1;
	while (!trump.empty()) {
		auto next_trump = trump.back();
		trump.pop_back();
		if (is_mit != hand_and_skat.contains(next_trump)) {
			break;
		}
		++spitzen;
	}
	return spitzen;
}

struct Contract{
	GameType game;
	bool hand;
	bool schneider;
	bool schwarz;
	bool ouvert;
};

[[nodiscard]] inline auto get_base_value(GameType game) -> int {
	switch (game) {
		case GameType::Grand: return 24;
		case GameType::Eichel: return 12;
		case GameType::Green: return 11;
		case GameType::Herz: return 10;
		case GameType::Schell: return 9;
		case GameType::Null: ;
	}
	
	assert(false); return 0;
}

//Not influenced by overbidding, or by the question whether the contract was fulfilled or not.
[[nodiscard]] inline auto get_contract_value(
	Contract contract,
	int spitzen,
	Score final_score
) -> int {
	if (contract.game == GameType::Null) {
		return contract.hand
			? (contract.ouvert ? 59 : 35)
			: (contract.ouvert ? 46 : 23);
	}

	auto base_value = get_base_value(contract.game);
	
	//Mit/Ohne n.
	auto multiplier = spitzen;
	
	//Spiel n + 1.
	++multiplier;
	
	//Hand?
	if (contract.hand) { ++multiplier; }
	
	//Schneider? Angesagt?
	if (contract.schneider) {
		multiplier += 2;
	} else {
		if (someone_is_schneider(final_score)) {
			++multiplier;
		}
	}

	//Schwarz? Angesagt?
	if (contract.schwarz) {
		multiplier += 2;
	} else {
		if (someone_is_schwarz(final_score)) {
			++multiplier;
		}
	}

	//Ouvert?
	if (contract.ouvert) { ++multiplier; }

	return multiplier * base_value;
}


[[nodiscard]] inline auto get_increased_value_due_to_overbid(
	GameType game,
	int spitzen,
	int bidding_value
) -> int {
	assert(bidding_value > 0);

	if (game == GameType::Null) {
		//Sigh.
		return std::min({
			get_increased_value_due_to_overbid(GameType::Eichel, spitzen, bidding_value),
			get_increased_value_due_to_overbid(GameType::Schell, spitzen, bidding_value),
			get_increased_value_due_to_overbid(GameType::Herz, spitzen, bidding_value),
			get_increased_value_due_to_overbid(GameType::Green, spitzen, bidding_value),
			get_increased_value_due_to_overbid(GameType::Grand, spitzen, bidding_value)
		});
	}

	auto base_value = get_base_value(game);

	auto multiplier_to_reach_bid = bidding_value / base_value + (bidding_value % base_value != 0);
	auto min_multiplier = spitzen + 1;

	auto result = std::max(multiplier_to_reach_bid, min_multiplier) * base_value;
	assert(bidding_value <= result);
	return result;
}

[[nodiscard]] inline auto is_fulfilled(
	Contract contract,
	Score final_score
) -> bool {
	if (contract.game == GameType::Null) {
		return final_score == Score{0, 0};
	}

	if (contract.schwarz) {
		return final_score == Score{120, 10};
	}

	if (contract.schneider) {
		return 90 <= final_score.points();
	}

	return 61 <= final_score.points();
}


//value: what the game is worth (always positive, might be increased due to overbidding)
//is_won: If the game was won (could be false due to overbidding, or because the contract was not fulfilled)
//These values can be used by scoring systems to determine how the score of the declarer is updated.
struct GameResult {
	int value;
	bool is_won;
};

[[nodiscard]] inline auto get_game_result(
	Contract contract,
	int spitzen,
	int bidding_value,
	Score final_score
) -> GameResult {
	auto value = get_contract_value(contract, spitzen, final_score);

	auto is_won = [&](){
		if (value < bidding_value) {
			//Overbid!
			auto value_due_to_overbid = get_increased_value_due_to_overbid(contract.game, spitzen, bidding_value);
			assert(value < value_due_to_overbid);
			value = value_due_to_overbid;
			return false;
		}

		return is_fulfilled(contract, final_score);
	}();

	return GameResult{value, is_won};
}

[[nodiscard]] inline auto score_classical(GameResult result) {
	if (result.is_won) {
		return result.value;
	}
	return -(2 * result.value);
}

[[nodiscard]] inline auto score_seeger(GameResult result) {
	if (result.is_won) {
		return 50 + result.value;
	}
	return -(2 * result.value);
}

[[nodiscard]] inline auto score_seeger_fabian_3p(GameResult result) {
	if (result.is_won) {
		return 50 + result.value;
	}
	return -(40 + 2 * result.value);
}

[[nodiscard]] inline auto score_seeger_fabian_4p(GameResult result) {
	if (result.is_won) {
		return 50 + result.value;
	}
	return -(30 + 2 * result.value);
}


} // namespace muskat
