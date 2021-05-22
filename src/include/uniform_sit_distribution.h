#pragma once


#include "contract.h"
#include "world_simulation.h"

#include <random>

#include <stdc/type_traits.h>

namespace muskat {

using WeirdDistResult = std::pair<std::tuple<Situation, Card, Card, GameType>, int>;

namespace detail {

template<typename ...Dist>
[[nodiscard]] auto combine_distributions(Dist ...distributions) {
	static_assert(stdc::are_same_v<std::array<Cards, 4>, Dist ...>);
	auto dec = (... | distributions[0]);
	auto fdef = (... | distributions[1]);
	auto sdef = (... | distributions[2]);
	auto skat = (... | distributions[3]);

	assert(dec.size() == 10);
	assert(fdef.size() == 10);
	assert(sdef.size() == 10);
	assert(skat.size() == 2);
	assert((dec | fdef | sdef | skat) == ~Cards{});
	return std::array{dec, fdef, sdef, skat};
	
	
}

[[nodiscard]] inline auto get_all_distributions_to_buckets(
	Cards cards_to_distribute,
	std::array<uint8_t, 4> sizes
) {
	using namespace stdc::literals;

	auto size = static_cast<uint8_t>(cards_to_distribute.size());
	assert(size = std::accumulate(RANGE(sizes), uint8_t{}));
	auto result = std::vector<std::array<Cards, 4>>{};
	result.reserve(multichoose(size, sizes));

	auto distribute_next_card_or_add_to_result = [&](
		std::array<Cards, 4> distribution_so_far,
		Cards remaining_cards,
		auto self
	) {
		if (remaining_cards.empty()) {
			result.push_back(distribution_so_far);
			return;
		}

		auto next_card = remaining_cards.remove_next();

		for (auto i = 0_z; i < 4; ++i) {
			auto cards = distribution_so_far[i];
			if (cards.size() == sizes[i]) {
				continue;
			}
			cards.add(next_card);
			auto new_distribution = distribution_so_far;
			new_distribution[i] = cards;
			self(new_distribution, remaining_cards, self);
		}
	};
	
	auto initial = std::array<Cards, 4>{Cards{}, Cards{}, Cards{}, Cards{}};
	distribute_next_card_or_add_to_result(initial, cards_to_distribute, distribute_next_card_or_add_to_result);
	assert(result.size() == multichoose(size, sizes));
	return result;
}

} //namespace detail


[[nodiscard]] inline auto get_signatures_dec_fdef_sdef_skat_and_entropy_and_number_of_possibilities(
	std::array<Cards, 5> unknown_cards_per_trick_type,
	std::array<KnownUnknownInSet, 4> known_about_unknown_dec_fdef_sdef_skat

) -> std::pair<
	std::vector<std::pair<
		std::array<TrickTypeSignature, 4>,
		uint64_t
	>>,
	uint64_t
> {

	auto signatures_dec_fdef_sdef_skat_and_entropy = std::vector<std::pair<
		std::array<TrickTypeSignature, 4>,
		uint64_t
	>> {};

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
						[[maybe_unused]] auto remaining_after_all_is_distributed = remaining_unknown_after_distributing(remaining_4, dist_4, static_cast<TrickType>(4));
						assert(is_nothing_unknown_left(remaining_after_all_is_distributed));

						auto sig_0 = TrickTypeSignature{dist_0[0], dist_1[0], dist_2[0], dist_3[0], dist_4[0]};
						auto sig_1 = TrickTypeSignature{dist_0[1], dist_1[1], dist_2[1], dist_3[1], dist_4[1]};
						auto sig_2 = TrickTypeSignature{dist_0[2], dist_1[2], dist_2[2], dist_3[2], dist_4[2]};
						auto sig_3 = TrickTypeSignature{dist_0[3], dist_1[3], dist_2[3], dist_3[3], dist_4[3]};

						auto entropy = poss_0 * poss_1 * poss_2 * poss_3 * poss_4;

						signatures_dec_fdef_sdef_skat_and_entropy.emplace_back(
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
	assert(!signatures_dec_fdef_sdef_skat_and_entropy.empty());

	auto number_of_possibilities = stdc::transform_accumulate(
		RANGE(signatures_dec_fdef_sdef_skat_and_entropy),
		[](const auto &signatures_entropy) {
			return signatures_entropy.second;
		}
	);
	assert(number_of_possibilities != 0);
	static_assert(std::is_same_v<uint64_t, decltype(number_of_possibilities)>);
	return {signatures_dec_fdef_sdef_skat_and_entropy, number_of_possibilities};
}


class UniformInitialSitDistribution {
private:
	GameType game;
	Role active_role;

public:
	UniformInitialSitDistribution(GameType a_game, Role a_active_role) :
		game{a_game},
		active_role{a_active_role}
	{}

	template<typename Generator>
	[[nodiscard]] auto operator()(Generator &rng) const -> WeirdDistResult {
		using namespace stdc::literals;
		
		auto shuffled_deck = get_shuffled_deck(rng);

		//Not according to skat rules, since the declarer is always in a different initial position.
		//But shuffled is shuffled.
		auto [h_dec, h_fd, h_sd, skat] = deal_deck(shuffled_deck);

		//We force the skat to be gedrueckt to get an uniform distribution of skat games.
		//In reality of course, games are not distributed like that.
		//Similar with what game is declared by whom.
		auto gedrueckt = skat;
		
		assert(skat.size() == 2);
		auto skat_0 = skat.remove_next();
		auto skat_1 = skat.remove_next();

		auto cards_declarer = h_dec | gedrueckt;
		auto spitzen = get_spitzen(cards_declarer, game);
		
		return {{Situation{
			h_dec,
			h_fd,
			h_sd,
			gedrueckt,
			active_role,
			MaybeCard{},
			MaybeCard{}
		}, skat_0, skat_1, game}, spitzen};
	}
};

//TODO: Currently, worlds don't guarantee as invariant to have at least one possible situation.
class UniformSitDistribution{
private:
	std::vector<std::pair<
		std::array<TrickTypeSignature, 4>,
		uint64_t
	>> signatures_dec_fdef_sdef_skat_and_entropy;
	uint64_t number_of_possibilities;
	std::array<Cards, 5> unknown_cards_per_trick_type;
	std::array<Cards, 4> known_cards_dec_fdef_sdef_skat;
	Role active_role;
	MaybeCard maybe_first_trick_card;
	MaybeCard maybe_second_trick_card;
	GameType m_game;
	Cards m_already_played_cards_dec;


	template<typename RNG>
	[[nodiscard]] const auto &choose_signature_weighted(RNG &rng) const {
		using namespace stdc::literals;
		
		auto sig_id = 0_z;
		{
			auto number = std::uniform_int_distribution{uint64_t{1}, number_of_possibilities}(rng);
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
		return signatures_dec_fdef_sdef_skat_and_entropy[sig_id].first;
	}

	private:
	[[nodiscard]] auto create_weird_dist_result(
		std::array<Cards, 4> cards_for_simulator
	) const -> WeirdDistResult {
		auto skat = cards_for_simulator[3];
		assert(skat.size() == 2);
		auto skat_0 = skat.remove_next();
		auto skat_1 = skat.remove_next();

		auto h_dec_at_start_of_game = cards_for_simulator[0] | m_already_played_cards_dec;
		auto cards_declarer = h_dec_at_start_of_game | cards_for_simulator[3];
		auto spitzen = get_spitzen(cards_declarer, m_game);
		
		return {{Situation{
			cards_for_simulator[0],
			cards_for_simulator[1],
			cards_for_simulator[2],
			cards_for_simulator[3],
			active_role,
			maybe_first_trick_card,
			maybe_second_trick_card
		}, skat_0, skat_1, m_game}, spitzen};
	}

public:
	explicit UniformSitDistribution(
		PossibleWorlds worlds
	) {
		unknown_cards_per_trick_type = split_by_trick_type(worlds.unknown_cards, worlds.game);
		known_cards_dec_fdef_sdef_skat = worlds.known_cards_dec_fdef_sdef_skat;
		active_role = worlds.active_role;
		maybe_first_trick_card = worlds.maybe_first_trick_card;
		maybe_second_trick_card = worlds.maybe_second_trick_card;
		m_game = worlds.game;
		m_already_played_cards_dec = worlds.already_played_cards_dec;


		std::tie(signatures_dec_fdef_sdef_skat_and_entropy, number_of_possibilities) = get_signatures_dec_fdef_sdef_skat_and_entropy_and_number_of_possibilities(
			unknown_cards_per_trick_type,
			worlds.known_about_unknown_dec_fdef_sdef_skat
		);
	}

	[[nodiscard]] constexpr auto get_number_of_possibilities() const -> uint64_t {
		return number_of_possibilities;
	}

	[[nodiscard]] constexpr auto get_number_of_color_distributions() const -> size_t {
		return signatures_dec_fdef_sdef_skat_and_entropy.size();
	}

	template<typename Generator>
	[[nodiscard]] auto operator()(Generator &rng) const -> WeirdDistResult {
		using namespace stdc::literals;
		
		const auto &selected_signature = choose_signature_weighted(rng);

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

		return create_weird_dist_result(cards_for_simulator);
	}

	[[nodiscard]] auto get_all_possibilities() const -> std::vector<WeirdDistResult> {
		auto result = std::vector<WeirdDistResult>{};
		result.reserve(get_number_of_possibilities());

		auto get_dists = [&]<size_t I>(const auto &signature){
			static_assert(I < 5);
			return detail::get_all_distributions_to_buckets(
				unknown_cards_per_trick_type[I],
				std::array{
					signature[0][I],
					signature[1][I],
					signature[2][I],
					signature[3][I]
				}
			);
		};


		for (const auto &[signature, entropy] : signatures_dec_fdef_sdef_skat_and_entropy) {
			auto dists_0 = get_dists.operator()<0>(signature);
			auto dists_1 = get_dists.operator()<1>(signature);
			auto dists_2 = get_dists.operator()<2>(signature);
			auto dists_3 = get_dists.operator()<3>(signature);
			auto dists_4 = get_dists.operator()<4>(signature);
			
			assert(
				dists_0.size() * dists_1.size() * dists_2.size() * dists_3.size() * dists_4.size() ==
				entropy
			);
			
			for (auto dist_0 : dists_0) {
				for (auto dist_1 : dists_1) {
					for (auto dist_2 : dists_2) {
						for (auto dist_3 : dists_3) {
							for (auto dist_4 : dists_4) {
								auto cards_for_simulator = detail::combine_distributions(
									dist_0,
									dist_1,
									dist_2,
									dist_3,
									dist_4
								);
								result.push_back(create_weird_dist_result(cards_for_simulator));
							}
						}
					}
				}
			}

		}

		assert(result.size() == get_number_of_possibilities());
		return result;
	}


};



} // namespace muskat
