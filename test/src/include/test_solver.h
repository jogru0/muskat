#pragma once
#include <gtest/gtest.h>

#include "situation_solver.h"

#include <stdc/literals.h>
#include <stdc/mathematics.h>

inline void test_calculate_potential_score(
	muskat::Deck deck,
	muskat::GameType game,
	muskat::Role role_of_vorhand,
	muskat::Points expected
) {
	auto [hand_geber, hand_hoerer, hand_sager, skat] = deal_deck(deck);
	auto initial_situation = muskat::Situation{hand_geber, hand_hoerer, hand_sager, skat, role_of_vorhand};
	
	// std::cout << "declarer: " << to_string(hand_geber) << "\n";
	// std::cout << "first defender: " << to_string(hand_hoerer) << "\n";
	// std::cout << "second defender: " << to_string(hand_sager) << "\n";
	// std::cout << "skat: " << to_string(skat) << "\n";
	
	auto solver = muskat::SituationSolver{game};
	auto treshold = solver.calculate_potential_score(initial_situation);
	ASSERT_EQ(expected, treshold);
}

inline void test_calculate_potential_score_for_rng_series(
	muskat::GameType game,
	muskat::Role role_vorhand,
	std::vector<muskat::Points> expected_results
) {
	auto rng = stdc::seeded_RNG(stdc::DeterministicSourceOfRandomness<0, 77'777'777>{});

	for (auto expected_result : expected_results) {
		auto deck = muskat::get_shuffled_deck(rng);
		test_calculate_potential_score(deck, game, role_vorhand, expected_result);
	}
}


TEST(solver, calculate_potential_score_series_herz_vorhand) {
	test_calculate_potential_score_for_rng_series(
		muskat::GameType::Herz,
		muskat::Role::Declarer,
		std::vector<muskat::Points>{71, 0, 0, 6, 39}
	);
}

TEST(solver, calculate_potential_score_series_herz_mittelhand) {
	test_calculate_potential_score_for_rng_series(
		muskat::GameType::Herz,
		muskat::Role::SecondDefender,
		std::vector<muskat::Points>{61, 0, 0, 6, 32}
	);
}


TEST(solver, calculate_potential_score_series_herz_hinterhand) {
	test_calculate_potential_score_for_rng_series(
		muskat::GameType::Herz,
		muskat::Role::FirstDefender,
		std::vector<muskat::Points>{61, 0, 0, 6, 32}
	);
}

TEST(solver, calculate_potential_score_series_grand_vorhand) {
	test_calculate_potential_score_for_rng_series(
		muskat::GameType::Grand,
		muskat::Role::Declarer,
		std::vector<muskat::Points>{68, 11, 0, 10, 43}
	);
}

TEST(solver, calculate_potential_score_series_grand_mittelhand) {
	test_calculate_potential_score_for_rng_series(
		muskat::GameType::Grand,
		muskat::Role::SecondDefender,
		std::vector<muskat::Points>{36, 11, 0, 4, 43}
	);
}


TEST(solver, calculate_potential_score_series_grand_hinterhand) {
	test_calculate_potential_score_for_rng_series(
		muskat::GameType::Grand,
		muskat::Role::FirstDefender,
		std::vector<muskat::Points>{36, 11, 0, 4, 43}
	);
}
