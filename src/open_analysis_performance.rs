use std::{
    hash::{Hash, Hasher},
    io,
    iter::from_fn,
    time::Duration,
};

use rand::Rng;
use rustc_hash::FxHasher;

use crate::{
    bidding_role::BiddingRole,
    card::{CardType, Suit},
    deck::Deck,
    game_type::GameType,
    monte_carlo::GameConclusion,
    open_situation_solver::{
        OpenSituationSolver,
        bounds_cache::{FastOpenSituationSolverCache, open_situation_reachable_from_to_u32_key},
    },
    role::Role,
    situation::OpenSituation,
    stats::write_stats,
    trick_yield::YieldSoFar,
};

#[derive(Hash, Clone, Copy)]
pub struct OpenSituationAndGameType {
    open_situation: OpenSituation,
    game_type: GameType,
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

        Some(OpenSituationAndGameType {
            open_situation: OpenSituation::initial(random_deal, BiddingRole::FIRST_ACTIVE_PLAYER),
            game_type: GameType::Trump(random_trump_type),
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

pub struct SingleResult {
    nodes: usize,
    time: Duration,
    total_yield: YieldSoFar,
}

#[derive(Debug)]
pub enum Level {
    IsWon,
    Conclusion,
    Yield,
}

impl PerformanceResult {
    pub fn write(&self, wt: &mut impl io::Write) -> Result<(), io::Error> {
        writeln!(wt, "=====================-----=====================")?;
        writeln!(
            wt,
            "Performance of getting {:?} of {} samples:",
            self.level,
            self.nodes_vec.len()
        )?;

        write_stats(
            "Number of 1000 nodes",
            self.nodes_vec.iter().map(|&data| data as f64 / 1000.),
            wt,
        )?;

        write_stats(
            "Time spend in ms",
            self.time_vec.iter().map(|data| data.as_secs_f64() * 1000.0),
            wt,
        )?;

        writeln!(wt, "Hash in: {}\n", self.hash_in)?;

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

pub fn measure_performance_to_decide_winner_of_open_situations<F>(
    open_situation_and_game_type_iter: impl Iterator<Item = OpenSituationAndGameType>,
    analyzer: F,
    level: Level,
) -> PerformanceResult
where
    F: Fn(OpenSituationAndGameType) -> SingleResult,
{
    let mut hasher_in = FxHasher::default();
    let mut hasher_yield = FxHasher::default();
    let mut hasher_conclusion = FxHasher::default();
    let mut hasher_won = FxHasher::default();

    let mut nodes_vec = Vec::new();
    let mut time_vec = Vec::new();

    for open_situation_and_game_type in open_situation_and_game_type_iter {
        open_situation_and_game_type.hash(&mut hasher_in);

        debug_assert_eq!(
            open_situation_and_game_type.open_situation.active_role(),
            Role::Declarer
        );
        debug_assert!(!matches!(
            open_situation_and_game_type.game_type,
            GameType::Null
        ));

        let SingleResult {
            nodes,
            time,
            total_yield,
        } = analyzer(open_situation_and_game_type);
        let conclusion = GameConclusion::from_final_declarer_yield(&total_yield);
        let is_won = GameConclusion::DefendersAreDominated <= conclusion;

        total_yield.hash(&mut hasher_yield);
        conclusion.hash(&mut hasher_conclusion);
        is_won.hash(&mut hasher_won);

        nodes_vec.push(nodes);
        time_vec.push(time);
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

pub fn analyzer_is_won(
    OpenSituationAndGameType {
        open_situation,
        game_type,
    }: OpenSituationAndGameType,
) -> SingleResult {
    let mut solver = OpenSituationSolver::new(
        FastOpenSituationSolverCache::new(open_situation_reachable_from_to_u32_key(open_situation)),
        game_type,
    );

    let yield_from_skat = open_situation.yield_from_skat();
    let yield_goal = GameConclusion::DefendersAreDominated.least_yield_necessary_to_be_in_here();
    let threshold = yield_goal.saturating_sub(yield_from_skat);

    let is_won = solver.still_makes_at_least(open_situation, threshold);

    SingleResult {
        nodes: solver.nodes_generated(),
        time: solver.time_spent(),
        total_yield: if is_won { yield_goal } else { yield_from_skat },
    }
}

pub fn analyzer_conclusion(
    OpenSituationAndGameType {
        open_situation,
        game_type,
    }: OpenSituationAndGameType,
) -> SingleResult {
    let mut solver = OpenSituationSolver::new(
        FastOpenSituationSolverCache::new(open_situation_reachable_from_to_u32_key(open_situation)),
        game_type,
    );

    let yield_from_skat = open_situation.yield_from_skat();

    let mut conclusion = None;
    for game_conclusion in [
        GameConclusion::DefendersAreSchwarz,
        GameConclusion::DefendersAreSchneider,
        GameConclusion::DefendersAreDominated,
        GameConclusion::DeclarerIsDominated,
        GameConclusion::DeclarerIsSchneider,
    ] {
        let yield_goal = game_conclusion.least_yield_necessary_to_be_in_here();
        let threshold = yield_goal.saturating_sub(yield_from_skat);
        if solver.still_makes_at_least(open_situation, threshold) {
            conclusion = Some(game_conclusion);
            break;
        }
    }
    let conclusion = conclusion.unwrap_or(GameConclusion::DeclarerIsSchwarz);

    SingleResult {
        nodes: solver.nodes_generated(),
        time: solver.time_spent(),
        total_yield: conclusion.least_yield_necessary_to_be_in_here(),
    }
}

pub fn analyzer_yield(
    OpenSituationAndGameType {
        open_situation,
        game_type,
    }: OpenSituationAndGameType,
) -> SingleResult {
    let mut solver = OpenSituationSolver::new(
        FastOpenSituationSolverCache::new(open_situation_reachable_from_to_u32_key(open_situation)),
        game_type,
    );

    let yield_from_skat = open_situation.yield_from_skat();
    let future_yield =
        solver.calculate_future_yield_with_optimal_open_play(open_situation, yield_from_skat);
    let total_yield = yield_from_skat.add(future_yield);

    SingleResult {
        nodes: solver.nodes_generated(),
        time: solver.time_spent(),
        total_yield,
    }
}
