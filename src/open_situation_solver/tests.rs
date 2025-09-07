use rand::SeedableRng;
use rand_xoshiro::Xoshiro256PlusPlus;

use crate::{
    bidding_role::BiddingRole,
    card::{CardType, Suit},
    card_points::CardPoints,
    deck::Deck,
    game_type::GameType,
    open_analysis_performance::analyzer_yield,
    open_situation_solver::{
        OpenSituationSolver,
        bounds_cache::{FastOpenSituationSolverCache, open_situation_reachable_from_to_u32_key},
    },
    situation::OpenSituation,
    trick_yield::YieldSoFar,
};

fn test_calculate_potential_score(
    deck: Deck,
    game_type: GameType,
    declarer: BiddingRole,
    expected: YieldSoFar,
) {
    let deal = deck.deal();
    let open_situation = OpenSituation::initial(deal, declarer);
    // dbg!(open_situation);

    let mut solver = OpenSituationSolver::new(
        FastOpenSituationSolverCache::new(
            open_situation_reachable_from_to_u32_key(open_situation),
            game_type,
        ),
        game_type,
    );

    let actual = analyzer_yield(
        open_situation,
        &mut solver,
        open_situation.yield_from_skat(),
    );

    assert_eq!(actual, expected);
}

fn test_calculate_potential_score_rng_batch(
    game_type: GameType,
    declarer: BiddingRole,
    seed: u64,
    expecteds: &[YieldSoFar],
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
            YieldSoFar::new(CardPoints(9), 1),
            YieldSoFar::new(CardPoints(12), 1),
            YieldSoFar::new(CardPoints(47), 1),
            YieldSoFar::new(CardPoints(32), 1),
            YieldSoFar::new(CardPoints(45), 1),
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
            YieldSoFar::new(CardPoints(30), 1),
            YieldSoFar::new(CardPoints(27), 1),
            YieldSoFar::new(CardPoints(21), 0),
            YieldSoFar::new(CardPoints(36), 1),
            YieldSoFar::new(CardPoints(25), 1),
        ],
    );
}

#[test]
fn test_calculate_potential_score_null_second_caller() {
    test_calculate_potential_score_rng_batch(
        GameType::Null,
        BiddingRole::SecondCaller,
        333,
        &[
            YieldSoFar::new(CardPoints(91), 1),
            YieldSoFar::new(CardPoints(62), 1),
            YieldSoFar::new(CardPoints(60), 1),
            YieldSoFar::new(CardPoints(67), 1),
        ],
    );
}
