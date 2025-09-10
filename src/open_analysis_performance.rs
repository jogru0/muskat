use foldhash::fast::FixedState;
use rand::{Rng, seq::IteratorRandom};
use std::hash::BuildHasher;
use std::{
    hash::{Hash, Hasher},
    io,
    iter::from_fn,
    time::Duration,
};

use crate::{
    bidding_role::BiddingRole,
    card::{CardType, Suit},
    card_points::CardPoints,
    deck::Deck,
    game_type::GameType,
    monte_carlo::GameConclusion,
    open_situation_solver::{
        OpenSituationSolver,
        bounds_cache::{
            FastOpenSituationSolverCache, OpenSituationSolverCache,
            open_situation_reachable_from_to_u32_key,
        },
    },
    situation::OpenSituation,
    stats::write_node_timing_stats,
    trick_yield::YieldSoFar,
};

#[derive(Hash, Clone, Copy)]
pub struct OpenSituationAndGameType {
    open_situation: OpenSituation,
    game_type: GameType,
    yield_so_far: YieldSoFar,
}

pub fn generate_random_trump_games_of_active_player(
    rng: &mut impl Rng,
) -> impl Iterator<Item = OpenSituationAndGameType> {
    from_fn(|| {
        let random_deal = Deck::shuffled(rng).deal();
        let random_trump_type = match rng.random_range(0..5) {
            0 => CardType::Trump,
            1 => CardType::Suit(Suit::Clubs),
            2 => CardType::Suit(Suit::Diamonds),
            3 => CardType::Suit(Suit::Spades),
            4 => CardType::Suit(Suit::Hearts),
            _ => unreachable!("out of range"),
        };

        let open_situation = OpenSituation::initial(random_deal, BiddingRole::FIRST_ACTIVE_PLAYER);

        Some(OpenSituationAndGameType {
            open_situation,
            game_type: GameType::Trump(random_trump_type),
            yield_so_far: open_situation.yield_from_skat(),
        })
    })
}

pub fn generate_random_unfinished_open_situations(
    rng: &mut impl Rng,
) -> impl Iterator<Item = OpenSituationAndGameType> {
    from_fn(|| {
        let deal = Deck::shuffled(rng).deal();
        let game_type = match rng.random_range(0..2) {
            0 => GameType::Null,
            1 => {
                let card_type = match rng.random_range(0..2) {
                    0 => {
                        let suit = match rng.random_range(0..4) {
                            0 => Suit::Clubs,
                            1 => Suit::Hearts,
                            2 => Suit::Diamonds,
                            3 => Suit::Spades,
                            _ => unreachable!("out of range"),
                        };
                        CardType::Suit(suit)
                    }
                    1 => CardType::Trump,
                    _ => unreachable!("out of range"),
                };
                GameType::Trump(card_type)
            }
            _ => unreachable!("out of range"),
        };

        let bidding_winner = match rng.random_range(0..3) {
            0 => BiddingRole::FirstReceiver,
            1 => BiddingRole::FirstCaller,
            2 => BiddingRole::SecondCaller,
            _ => unreachable!("out of range"),
        };

        let mut open_situation = OpenSituation::initial(deal, bidding_winner);

        let mut yield_so_far = open_situation.yield_from_skat();

        // Up to 29 decisions, so there is one more left (where we can analyze all childs).
        for _ in 0..rng.random_range(0..30) {
            let possibilities_for_decision = open_situation.next_possible_plays(game_type);
            yield_so_far.add_assign(
                open_situation.play_card(
                    possibilities_for_decision
                        .choose(rng)
                        .expect("was choosen out of it"),
                    game_type,
                ),
            );
        }

        Some(OpenSituationAndGameType {
            open_situation,
            game_type,
            yield_so_far,
        })
    })
}

pub struct PerformanceResult {
    nodes_vec: Vec<usize>,
    time_vec: Vec<Duration>,
    hash_in: u64,
    hash_yield: u64,
    hash_conclusion: u64,
    hash_won: u64,
    level: Level,
}

#[derive(Debug, Clone, Copy)]
pub enum Level {
    IsWon,
    Conclusion,
    Yield,
}

impl PerformanceResult {
    pub fn write(&self, wt: &mut impl io::Write) -> Result<(), io::Error> {
        writeln!(wt, "==========================================")?;
        writeln!(
            wt,
            "Performance of getting {:?} of {} samples:",
            self.level,
            self.nodes_vec.len()
        )?;

        write_node_timing_stats(&self.nodes_vec, &self.time_vec, wt)?;

        writeln!(wt, "\nHash in: {}\n", self.hash_in)?;

        writeln!(wt, "Hash won: {}", self.hash_won)?;
        if !matches!(self.level, Level::IsWon) {
            writeln!(wt, "Hash conclusion: {}", self.hash_conclusion)?;
            if matches!(self.level, Level::Yield) {
                writeln!(wt, "Hash yield: {}", self.hash_yield)?;
            }
        }

        Ok(())
    }
}

pub fn measure_performance_to_decide_winner_of_open_situations(
    open_situation_and_game_type_iter: impl Iterator<Item = OpenSituationAndGameType>,
    level: Level,
) -> PerformanceResult {
    let fixed_state = FixedState::default();
    let mut hasher_in = fixed_state.build_hasher();
    let mut hasher_yield = fixed_state.build_hasher();
    let mut hasher_conclusion = fixed_state.build_hasher();
    let mut hasher_won = fixed_state.build_hasher();

    let mut nodes_vec = Vec::new();
    let mut time_vec = Vec::new();

    for open_situation_and_game_type in open_situation_and_game_type_iter {
        open_situation_and_game_type.hash(&mut hasher_in);

        let OpenSituationAndGameType {
            open_situation,
            game_type,
            yield_so_far,
        } = open_situation_and_game_type;

        let mut solver = OpenSituationSolver::new(
            FastOpenSituationSolverCache::new(
                open_situation_reachable_from_to_u32_key(open_situation),
                game_type,
            ),
            game_type,
        );

        let total_yield = analyzer(open_situation, &mut solver, yield_so_far, level);

        let conclusion = GameConclusion::from_final_declarer_yield(&total_yield);
        let is_won = GameConclusion::DefendersAreDominated <= conclusion;

        total_yield.hash(&mut hasher_yield);
        conclusion.hash(&mut hasher_conclusion);
        is_won.hash(&mut hasher_won);

        nodes_vec.push(solver.nodes_generated());
        time_vec.push(solver.time_spent());
    }

    PerformanceResult {
        nodes_vec,
        time_vec,
        hash_in: hasher_in.finish(),
        hash_yield: hasher_yield.finish(),
        hash_conclusion: hasher_conclusion.finish(),
        hash_won: hasher_won.finish(),
        level,
    }
}

pub fn measure_performance_to_judge_possible_next_turns_of_open_situation(
    open_situation_and_game_type_iter: impl Iterator<Item = OpenSituationAndGameType>,
    level: Level,
) -> PerformanceResult {
    let fixed_state = FixedState::default();
    let mut hasher_in = fixed_state.build_hasher();
    let mut hasher_yield = fixed_state.build_hasher();
    let mut hasher_conclusion = fixed_state.build_hasher();
    let mut hasher_won = fixed_state.build_hasher();

    let mut nodes_vec = Vec::new();
    let mut time_vec = Vec::new();

    for open_situation_and_game_type in open_situation_and_game_type_iter {
        open_situation_and_game_type.hash(&mut hasher_in);

        let OpenSituationAndGameType {
            open_situation,
            game_type,
            yield_so_far,
        } = open_situation_and_game_type;

        let mut solver = OpenSituationSolver::new(
            FastOpenSituationSolverCache::new(
                open_situation_reachable_from_to_u32_key(open_situation),
                game_type,
            ),
            game_type,
        );

        for card in open_situation.next_possible_plays(game_type) {
            let mut child = open_situation;

            let yield_so_far_child = yield_so_far.add(child.play_card(card, game_type));

            let total_yield = analyzer(child, &mut solver, yield_so_far_child, level);

            let conclusion = GameConclusion::from_final_declarer_yield(&total_yield);
            let is_won = GameConclusion::DefendersAreDominated <= conclusion;

            total_yield.hash(&mut hasher_yield);
            conclusion.hash(&mut hasher_conclusion);
            is_won.hash(&mut hasher_won);
        }

        nodes_vec.push(solver.nodes_generated());
        time_vec.push(solver.time_spent());
    }

    PerformanceResult {
        nodes_vec,
        time_vec,
        hash_in: hasher_in.finish(),
        hash_yield: hasher_yield.finish(),
        hash_conclusion: hasher_conclusion.finish(),
        hash_won: hasher_won.finish(),
        level,
    }
}

fn analyzer<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    solver: &mut OpenSituationSolver<C>,
    yield_so_far: YieldSoFar,
    level: Level,
) -> YieldSoFar {
    match level {
        Level::IsWon => analyzer_is_won(open_situation, solver, yield_so_far),
        Level::Conclusion => analyzer_conclusion(open_situation, solver, yield_so_far),
        Level::Yield => analyzer_yield(open_situation, solver, yield_so_far),
    }
}

fn analyzer_is_won<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    solver: &mut OpenSituationSolver<C>,
    yield_so_far: YieldSoFar,
) -> YieldSoFar {
    let yield_goal = GameConclusion::DefendersAreDominated.least_yield_necessary_to_be_in_here();
    let threshold = yield_goal.saturating_sub(yield_so_far);

    if solver.still_makes_at_least(open_situation, threshold) {
        yield_goal
    } else {
        yield_so_far
    }
}

fn analyzer_conclusion<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    solver: &mut OpenSituationSolver<C>,
    yield_so_far: YieldSoFar,
) -> YieldSoFar {
    let mut conclusion = None;
    for game_conclusion in [
        GameConclusion::DefendersAreSchwarz,
        GameConclusion::DefendersAreSchneider,
        GameConclusion::DefendersAreDominated,
        GameConclusion::DeclarerIsDominated,
        GameConclusion::DeclarerIsSchneider,
    ] {
        let yield_goal = game_conclusion.least_yield_necessary_to_be_in_here();
        let threshold = yield_goal.saturating_sub(yield_so_far);
        if solver.still_makes_at_least(open_situation, threshold) {
            conclusion = Some(game_conclusion);
            break;
        }
    }
    let conclusion = conclusion.unwrap_or(GameConclusion::DeclarerIsSchwarz);

    conclusion.least_yield_necessary_to_be_in_here()
}

pub fn analyzer_yield<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    solver: &mut OpenSituationSolver<C>,
    yield_so_far: YieldSoFar,
) -> YieldSoFar {
    let mut yield_goal = YieldSoFar::MAX;

    loop {
        let threshold = yield_goal.saturating_sub(yield_so_far);

        if solver.still_makes_at_least(open_situation, threshold) {
            return yield_goal;
        }

        if yield_goal == YieldSoFar::new(CardPoints(0), 1) {
            return yield_so_far;
        }

        yield_goal = YieldSoFar::new(CardPoints(yield_goal.card_points().0 - 1), 1)
    }
}
