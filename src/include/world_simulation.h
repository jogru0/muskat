#pragma once

#include "situation.h"

#include <stdc/mathematics.h>

#include <robin_hood/robin_hood.h>


//TODO: Verify with worst cases from paper that our clever algorithm does the right thing.
//TODO: Assert that everything is known that the active player should know, and that
//the possible plays don't depend on the specific situation created.

namespace muskat {

struct KnownUnknownInSet {
	uint8_t number;
	std::array<bool, 5> can_be_trick_type;
};

using TrickTypeSignature = std::array<uint8_t, 5>;

[[nodiscard]] inline auto choose(
	uint8_t n,
	uint8_t k
) -> uint64_t {
	assert(n >= k);
	k = std::min(k, uint8_t(n - k));
	auto numerator = uint64_t{1};
	auto denominator = uint64_t{1};
	for (auto i = uint8_t{1}; i <= k; ++i) {
		numerator *= n + 1 - i;
		denominator *= i;
	}
	assert(numerator % denominator == 0);
	return numerator / denominator;
}

[[nodiscard]] inline auto distribute(
	std::array<KnownUnknownInSet, 4> known_about_remaining_dec_fdef_sdef_skat,
	size_t number_to_distribute,
	TrickType trick_type
) {
	//All cards to distribute are of one trick type.
	assert(number_to_distribute <= 11);

	auto result = std::vector<std::pair<
		std::array<uint8_t, 4>, //distribution
		uint64_t //possibilities
	>>{};

	auto remaining_0 = static_cast<uint8_t>(number_to_distribute);
	const auto &[number_0, can_be_tt_0] = known_about_remaining_dec_fdef_sdef_skat[0];
	auto max_0 = can_be_tt_0[static_cast<size_t>(trick_type)] ? std::min(number_0, remaining_0) : uint8_t{0};
	for (auto i_0 = uint8_t{0}; i_0 <= max_0; ++i_0) {
		auto pos_0 = choose(remaining_0, i_0);
		auto remaining_1 = static_cast<uint8_t>(remaining_0 - i_0);
		const auto &[number_1, can_be_tt_1] = known_about_remaining_dec_fdef_sdef_skat[1];
		auto max_1 = can_be_tt_1[static_cast<size_t>(trick_type)] ? std::min(number_1, remaining_1) : uint8_t{0};
		for (auto i_1 = uint8_t{0}; i_1 <= max_1; ++i_1) {
			auto pos_1 = pos_0 * choose(remaining_1, i_1);
			auto remaining_2 = static_cast<uint8_t>(remaining_1 - i_1);
			const auto &[number_2, can_be_tt_2] = known_about_remaining_dec_fdef_sdef_skat[2];
			auto max_2 = can_be_tt_2[static_cast<size_t>(trick_type)] ? std::min(number_2, remaining_2) : uint8_t{0};
			for (auto i_2 = uint8_t{0}; i_2 <= max_2; ++i_2) {
				auto pos_2 = pos_1 * choose(remaining_2, i_2);
				auto remaining_3 = static_cast<uint8_t>(remaining_2 - i_2);
				const auto &[number_3, can_be_tt_3] = known_about_remaining_dec_fdef_sdef_skat[3];
				auto max_3_independent_of_remaining = can_be_tt_3[static_cast<size_t>(trick_type)] ? number_3 : uint8_t{0};
				if (remaining_3 > max_3_independent_of_remaining) {
					//Distributed too few cards of this trick type to the other sets
					//(maybe this was unavoidable because of how we distributed cards of other trick types),
					//so now we can't give all the rest to the last set.
					continue;
				}

				//Have to give all remaining cards to the last set,
				//otherwise not all cards of this trick type would have been distributed.
				const auto &i_3 = remaining_3;
				const auto &pos_3 = pos_2;

				result.emplace_back(
					std::array{i_0, i_1, i_2, i_3},
					pos_3
				);
			}
		}
	}

	return result;
}

[[nodiscard]] inline constexpr auto remaining_unknown_after_distributing(
	std::array<KnownUnknownInSet, 4> known_about_unknown_dec_fdef_sdef_skat,
	const std::array<uint8_t, 4> &distributed_numbers,
	TrickType distributed_trick_type
) {
	using namespace stdc::literals;

	for (auto set_id = 0_z; set_id < 4; ++set_id) {
		auto &known_about_unknown = known_about_unknown_dec_fdef_sdef_skat[set_id];
		const auto & distributed_number = distributed_numbers[set_id];
		assert(IMPLIES(
			distributed_number != 0,
			known_about_unknown.can_be_trick_type[static_cast<size_t>(distributed_trick_type)]
		));
		assert(distributed_number <= known_about_unknown.number);
		known_about_unknown.number -= distributed_number;
	}

	return known_about_unknown_dec_fdef_sdef_skat;
}

[[nodiscard]] inline constexpr auto is_nothing_unknown_left(
	std::array<KnownUnknownInSet, 4> known_about_unknown_dec_fdef_sdef_skat
) {
	using namespace stdc::literals;

	for (auto set_id = 0_z; set_id < 4; ++set_id) {
		if (known_about_unknown_dec_fdef_sdef_skat[set_id].number != 0) {
			return false;
		}
	}

	return true;
}


class PossibleWorlds {
private:
	std::array<KnownUnknownInSet, 4> known_about_unknown_dec_fdef_sdef_skat;
	std::array<Cards, 4> known_cards_dec_fdef_sdef_skat;
	Cards unknown_cards;
	GameType game;
	Role active_role;
	MaybeCard maybe_first_trick_card;
	MaybeCard maybe_second_trick_card;

public:
	PossibleWorlds(
		std::array<KnownUnknownInSet, 4> a_known_about_unknown_dec_fdef_sdef_skat,
		std::array<Cards, 4> a_known_cards_dec_fdef_sdef_skat,
		Cards a_unknown_cards,
		GameType a_game,
		Role a_active_role,
		MaybeCard a_maybe_first_trick_card,
		MaybeCard a_maybe_second_trick_card
	) :
		known_about_unknown_dec_fdef_sdef_skat{std::move(a_known_about_unknown_dec_fdef_sdef_skat)},
		known_cards_dec_fdef_sdef_skat{std::move(a_known_cards_dec_fdef_sdef_skat)},
		unknown_cards{std::move(a_unknown_cards)},
		game{std::move(a_game)},
		active_role{std::move(a_active_role)},
		maybe_first_trick_card{std::move(a_maybe_first_trick_card)},
		maybe_second_trick_card{std::move(a_maybe_second_trick_card)}
	{}

public:

	[[nodiscard]] auto get_game_type() const {
		return game;
	}

	template<typename RNG>
	[[nodiscard]] auto get_one_uniformly(RNG &rng) const -> Situation {
		using namespace stdc::literals;
		
		//Not directly known_cards_dec_fdef_sdef_skat because we mutate it.
		std::array<Cards, 4> cards_for_simulator;

		//Once done == true, all the cards_for_simulator ae set.
		auto done = false;
		while(!done) {
			done = [&](){
				auto cards_to_distribute = get_shuffled(unknown_cards, rng);
				for (auto pot_id = 0_z; pot_id < 4; ++pot_id) {
					auto &cards_pot_id = cards_for_simulator[pot_id];
					cards_pot_id = known_cards_dec_fdef_sdef_skat[pot_id];
					auto [number, can_be_trick_type] = known_about_unknown_dec_fdef_sdef_skat[pot_id];
					while (number --> 0) {
						assert(!cards_to_distribute.empty());
						auto card_to_add = cards_to_distribute.back();
						cards_to_distribute.pop_back();
						if (!can_be_trick_type[static_cast<size_t>(get_trick_type(card_to_add, game))]) {
							return false;
						}
						cards_pot_id.add(card_to_add);
					}
				}
				assert(cards_to_distribute.empty());
				return true;
			}();
		}

		return Situation{
			cards_for_simulator[0],
			cards_for_simulator[1],
			cards_for_simulator[2],
			cards_for_simulator[3],
			active_role,
			maybe_first_trick_card,
			maybe_second_trick_card
		};
	}

	[[nodiscard]] auto get_all_signatures_dec_fdef_sdef_skat_and_entropy() const -> std::vector<
		std::pair<
			std::array<TrickTypeSignature, 4>,
			uint64_t //Enough to hold the number of possible distributions when nothing is known.
		>
	> {

		auto result = std::vector<std::pair<
			std::array<TrickTypeSignature, 4>,
			uint64_t
		>>{};

		//TODO: Also done in get_one_uniformly_clever.
		auto unknown_cards_per_trick_type = split_by_trick_type(unknown_cards, game);
		auto unknown_number_per_trick_type = std::array{
			unknown_cards_per_trick_type[0].size(),
			unknown_cards_per_trick_type[1].size(),
			unknown_cards_per_trick_type[2].size(),
			unknown_cards_per_trick_type[3].size(),
			unknown_cards_per_trick_type[4].size()
		};
		
		const auto &remaining_0 = known_about_unknown_dec_fdef_sdef_skat;
		auto vec_distribution_and_possibilities_0 = distribute(remaining_0, unknown_number_per_trick_type[0], static_cast<TrickType>(0));
		for (const auto &[dist_0, poss_0] : vec_distribution_and_possibilities_0) {
			
			auto remaining_1 = remaining_unknown_after_distributing(remaining_0, dist_0, static_cast<TrickType>(0));
			auto vec_distribution_and_possibilities_1 = distribute(remaining_1, unknown_number_per_trick_type[1], static_cast<TrickType>(1));
			for (const auto &[dist_1, poss_1] : vec_distribution_and_possibilities_1) {
			
				auto remaining_2 = remaining_unknown_after_distributing(remaining_1, dist_1, static_cast<TrickType>(1));
				auto vec_distribution_and_possibilities_2 = distribute(remaining_2, unknown_number_per_trick_type[2], static_cast<TrickType>(2));
				for (const auto &[dist_2, poss_2] : vec_distribution_and_possibilities_2) {
				
					auto remaining_3 = remaining_unknown_after_distributing(remaining_2, dist_2, static_cast<TrickType>(2));
					auto vec_distribution_and_possibilities_3 = distribute(remaining_3, unknown_number_per_trick_type[3], static_cast<TrickType>(3));
					for (const auto &[dist_3, poss_3] : vec_distribution_and_possibilities_3) {
					
						auto remaining_4 = remaining_unknown_after_distributing(remaining_3, dist_3, static_cast<TrickType>(3));
						auto vec_distribution_and_possibilities_4 = distribute(remaining_4, unknown_number_per_trick_type[4], static_cast<TrickType>(4));
						//Either all remaining cards can be trump, or not.
						assert(vec_distribution_and_possibilities_4.size() <= 1);
						for (const auto &[dist_4, poss_4] : vec_distribution_and_possibilities_4) {
							auto remaining_after_all_is_distributed = remaining_unknown_after_distributing(remaining_4, dist_4, static_cast<TrickType>(4));
							assert(is_nothing_unknown_left(remaining_after_all_is_distributed));

							auto sig_0 = TrickTypeSignature{dist_0[0], dist_1[0], dist_2[0], dist_3[0], dist_4[0]};
							auto sig_1 = TrickTypeSignature{dist_0[1], dist_1[1], dist_2[1], dist_3[1], dist_4[1]};
							auto sig_2 = TrickTypeSignature{dist_0[2], dist_1[2], dist_2[2], dist_3[2], dist_4[2]};
							auto sig_3 = TrickTypeSignature{dist_0[3], dist_1[3], dist_2[3], dist_3[3], dist_4[3]};

							auto entropy = poss_0 * poss_1 * poss_2 * poss_3 * poss_4;

							result.emplace_back(
								std::array{sig_0, sig_1, sig_2, sig_3},
								entropy
							);
						}
					}
				}
			}
		}

		//At least one solution has to be there, because we only call this with inputs from actual skat games,
		//which of course always have an actual concrete distribution of cards.
		assert(!result.empty());
		return result;
	}


	//Many things can be calculated independently of the RNG.
	template<typename RNG>
	[[nodiscard]] auto get_one_uniformly_clever(RNG &rng) const -> Situation {
		using namespace stdc::literals;
		
		auto signatures_dec_fdef_sdef_skat_and_entropy = get_all_signatures_dec_fdef_sdef_skat_and_entropy();

		auto number_of_possibilities = stdc::transform_accumulate(
			RANGE(signatures_dec_fdef_sdef_skat_and_entropy),
			[](const auto &signatures_entropy) {
				return signatures_entropy.second;
			}
		);

		assert(number_of_possibilities != 0);
		static_assert(std::is_same_v<uint64_t, decltype(number_of_possibilities)>);
		auto dist = std::uniform_int_distribution{uint64_t{1}, number_of_possibilities};
	
		auto sig_id = 0_z;
		{
			auto number = dist(rng);
			auto partial_sum = uint64_t{0_z};
			for (;;) {
				assert(sig_id < signatures_dec_fdef_sdef_skat_and_entropy.size());
				partial_sum += signatures_dec_fdef_sdef_skat_and_entropy[sig_id].second;
				if (partial_sum >= number) {
					break;
				}
				++sig_id;
			}
		}
		const auto &selected_signature = signatures_dec_fdef_sdef_skat_and_entropy[sig_id].first;

		auto unknown_cards_per_trick_type = split_by_trick_type(unknown_cards, game);
		std::array<std::vector<Card>, 5> cards_to_distribute_by_trick_type;
		for (auto tt = 0_z; tt < 5; ++tt) {
			cards_to_distribute_by_trick_type[tt] = get_shuffled(unknown_cards_per_trick_type[tt], rng);
		}

		//Not just a reference to known_cards_dec_fdef_sdef_skat because we mutate it.
		auto cards_for_simulator = known_cards_dec_fdef_sdef_skat;

		for (auto i = 0_z; i < 4; ++i) {
			const auto &selected_signature_this_set = selected_signature[i];
			auto &cards_to_add_to = cards_for_simulator[i];
			for (auto tt = 0_z; tt < 5; ++tt) {
				

				auto number_of_cards_to_add = selected_signature_this_set[tt];
				auto &cards_to_distribute_with_trick_type = cards_to_distribute_by_trick_type[tt];
				while (number_of_cards_to_add --> 0) {
					assert(!cards_to_distribute_with_trick_type.empty());
					auto card_to_add = cards_to_distribute_with_trick_type.back();
					cards_to_distribute_with_trick_type.pop_back();
					cards_to_add_to.add(card_to_add);
				}
			}
		}


		for (auto tt = 0_z; tt < 5; ++tt) {
			assert(cards_to_distribute_by_trick_type[tt].empty());
		}
		
		return Situation{
			cards_for_simulator[0],
			cards_for_simulator[1],
			cards_for_simulator[2],
			cards_for_simulator[3],
			active_role,
			maybe_first_trick_card,
			maybe_second_trick_card
		};
	}
};

// [[nodiscard]] auto partition_by_trick_type(Cards cards, GameType game) {
// 	auto heart_cards = cards & cards_following_trick_type(TrickAndGameType{TrickType::Herz, game}
// }


// [[nodiscard]] auto distribute_missing_cards(Cards known_cards, std::vector<UnknownCards> vec_of_unknown_cards) {
// 	auto number_of_unknown_cards = stdc::transform_accumulate(RANGE(vec_of_unknown_cards), [](auto unknown_cards) {
// 		return unknown_cards.number;
// 	});
// 	assert(known_cards.size() + number_of_unknown_cards == 32);

// 	auto known_by_trick_types = partition_by_trick_type(known_cards);
// }

// [[nodiscard]] auto fake_


// [[nodiscard]] auto multithreaded_world_simulation(Situation sit) {
// 	auto number_of_threads = std::thread::hardware_concurrency();
// 	if (number_of_threads == 0) {
// 		throw std::runtime_error{"Could not determine best number of threads."};
// 	}

// 	std::cout << "Number of threads: " << number_of_threads << ".\n";

// 	auto do_stuff = [] (std::stop_token stoken, std::vector<std::array<Points, 32>> &results) {
// 		assert(results.empty());
		
// 		auto rng = stdc::seeded_RNG();
		
// 		while(!stoken.stop_requested()) {
// 			auto deck = muskat::get_shuffled_deck(rng);
// 			auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
// 			auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_vorhand};
		
			
// 		}
// 		for(int i=0; i < 10; i++) {
// 			std::this_thread::sleep_for(300ms);
// 			if() {
// 				std::cout << "Sleepy worker is requested to stop\n";
// 				return;
// 			}
// 			std::cout << "Sleepy worker goes back to sleep\n";
// 		}
// 	}

// }


} // namespace muskat
