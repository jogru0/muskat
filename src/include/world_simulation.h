#pragma once

#include "situation.h"

#include <stdc/mathematics.h>

#include <robin_hood/robin_hood.h>

namespace muskat {

struct KnownUnknownInSet {
	uint8_t number;
	std::array<bool, 5> can_be_trick_type;
};

using TrickTypeSignature = std::array<uint8_t, 5>;

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
		assert(false);
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
				partial_sum += signatures_dec_fdef_sdef_skat_and_entropy[sig_id].second;
				if (partial_sum >= number) {
					break;
				}
				++sig_id;
				assert(sig_id < signatures_dec_fdef_sdef_skat_and_entropy.size());
			}
		}
		const auto &selected_signature = signatures_dec_fdef_sdef_skat_and_entropy[sig_id].first;

		auto unknown_cards_per_trick_type = split_by_trick_type(unknown_cards, game);
		std::array<std::vector<Card>, 5> cards_to_distribute_by_trick_type;
		for (auto tt = 0_z; tt <  5; ++tt) {
			cards_to_distribute_by_trick_type[tt] = get_shuffled(unknown_cards_per_trick_type[tt], rng);
		}

		//Not directly known_cards_dec_fdef_sdef_skat because we mutate it.
		std::array<Cards, 4> cards_for_simulator = known_cards_dec_fdef_sdef_skat;

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
			assert(!cards_to_distribute_by_trick_type[tt].empty());
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
