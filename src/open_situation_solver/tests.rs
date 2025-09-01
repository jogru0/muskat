use rand::SeedableRng;
use rand_xoshiro::Xoshiro256PlusPlus;

use crate::{
    bidding_role::BiddingRole,
    card::{CardType, Suit},
    card_points::CardPoints,
    deck::Deck,
    game_type::GameType,
    open_situation_solver::{
        OpenSituationSolver,
        bounds_cache::{FastOpenSituationSolverCache, open_situation_reachable_from_to_u32_key},
    },
    situation::OpenSituation,
    trick_yield::{TrickYield, YieldSoFar},
};

fn test_calculate_potential_score(
    deck: Deck,
    game_type: GameType,
    declarer: BiddingRole,
    expected: TrickYield,
) {
    let deal = deck.deal();
    let open_situation = OpenSituation::initial(deal, declarer);
    // dbg!(open_situation);

    let mut solver = OpenSituationSolver::new(
        FastOpenSituationSolverCache::new(open_situation_reachable_from_to_u32_key(open_situation)),
        game_type,
    );

    let actual = solver.calculate_future_yield_with_optimal_open_play(
        open_situation,
        YieldSoFar::new(open_situation.cellar().to_points(), 0),
    );

    assert_eq!(actual, expected);
}

fn test_calculate_potential_score_rng_batch(
    game_type: GameType,
    declarer: BiddingRole,
    seed: u64,
    expecteds: &[TrickYield],
) {
    let mut rng = Xoshiro256PlusPlus::seed_from_u64(seed);

    for &expected in expecteds {
        let deck = Deck::shuffled(&mut rng);
        test_calculate_potential_score(deck, game_type, declarer, expected);
    }
}

#[test]
fn test_calculate_potential_score_suit_declarer() {
    test_calculate_potential_score_rng_batch(
        GameType::Trump(CardType::Suit(Suit::Hearts)),
        BiddingRole::FirstCaller,
        432,
        &[
            TrickYield::new(CardPoints(25), 1),
            TrickYield::new(CardPoints(14), 1),
            TrickYield::new(CardPoints(59), 1),
            TrickYield::new(CardPoints(30), 1),
            TrickYield::new(CardPoints(0), 0),
        ],
    );
}

#[test]
fn test_calculate_potential_score_grand_first_receiver() {
    test_calculate_potential_score_rng_batch(
        GameType::Trump(CardType::Trump),
        BiddingRole::FirstReceiver,
        32,
        &[
            TrickYield::new(CardPoints(6), 1),
            TrickYield::new(CardPoints(21), 1),
            TrickYield::new(CardPoints(11), 1),
            TrickYield::new(CardPoints(30), 1),
            TrickYield::new(CardPoints(10), 1),
        ],
    );
}

#[test]
fn test_calculate_potential_score_null_second_caller() {
    test_calculate_potential_score_rng_batch(
        GameType::Null,
        BiddingRole::SecondCaller,
        3,
        &[
            TrickYield::new(CardPoints(0), 0),
            TrickYield::new(CardPoints(0), 1),
            TrickYield::new(CardPoints(0), 1),
            TrickYield::new(CardPoints(0), 1),
            TrickYield::new(CardPoints(0), 1),
        ],
    );
}
