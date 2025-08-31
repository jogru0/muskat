use crate::{
    cards::Cards,
    dist::UniformPossibleDealsFromObservedGameplay,
    observed_gameplay::{
        CardKnowledge, ObservedInitialGameState, ObservedPlayedCards, OpenGameState,
    },
    open_situation_analyzer::{
        AnalyzedPossiblePlays, score_for_possible_plays::final_declarer_yield_for_possible_plays,
    },
    open_situation_solver::{
        OpenSituationSolver,
        bounds_cache::{FastOpenSituationSolverCache, open_situation_reachable_from_to_u32_key},
    },
    trick_yield::YieldSoFar,
};
use itertools::Itertools;
use rand::{Rng, distr::Distribution};
use rayon::iter::{IntoParallelRefIterator, ParallelIterator as _};
use std::{
    io::{self},
    time::Instant,
};

pub enum GameConclusion {
    DeclarerIsSchwarz,
    DeclarerIsSchneider,
    DeclarerIsDominated,
    DefendersAreDominated,
    DefenderAreSchneider,
    DefendersAreSchwarz,
}

impl GameConclusion {
    pub fn from_final_declarer_yield(final_declarer_yield: &YieldSoFar) -> Self {
        match (
            final_declarer_yield.card_points().0,
            final_declarer_yield.number_of_tricks(),
        ) {
            (_, 0) => Self::DeclarerIsSchwarz,
            (0..=30, 1..=9) => Self::DeclarerIsSchneider,
            (31..=60, 1..=9) => Self::DeclarerIsDominated,
            (61..=89, 1..=9) => Self::DefendersAreDominated,
            (90..=118, 1..=9) => Self::DefenderAreSchneider,
            (_, 10) => Self::DefendersAreSchwarz,
            _ => panic!("invalid: {final_declarer_yield:?}"),
        }
    }
}

#[derive(Debug)]
pub struct PossibleWorldData<A> {
    game_conclusion_for_plays: AnalyzedPossiblePlays<A>,
    // TODO: Proper type
    matadors: Option<u8>,
    probability_weight: f64,
}

#[derive(Debug)]
pub struct SampledWorldsData<A> {
    // Never empty. Consistend possible plays.
    possible_world_data_vec: Vec<PossibleWorldData<A>>,
}

impl<A> SampledWorldsData<A> {
    pub fn weighted_average<F>(&self, f: F) -> AnalyzedPossiblePlays<f64>
    where
        F: Fn(&A, Option<u8>) -> f64,
    {
        let sum_of_probability_weights = self.sum_of_probability_weights();

        self.possible_world_data_vec
            .iter()
            .map(|possible_world_data| {
                possible_world_data
                    .game_conclusion_for_plays
                    .map(|game_conclusion| {
                        f(game_conclusion, possible_world_data.matadors)
                            * possible_world_data.probability_weight
                    })
            })
            .sum::<AnalyzedPossiblePlays<f64>>()
            .map(|accumulated| accumulated / sum_of_probability_weights)
    }

    fn sum_of_probability_weights(&self) -> f64 {
        self.possible_world_data_vec
            .iter()
            .map(|possible_world_dat| possible_world_dat.probability_weight)
            .sum()
    }

    pub fn weighted_probability_of<F>(&self, f: F) -> AnalyzedPossiblePlays<f64>
    where
        F: Fn(&A, Option<u8>) -> bool,
    {
        self.weighted_average(|game_conclusion, matador| {
            if f(game_conclusion, matador) {
                1.0
            } else {
                0.0
            }
        })
    }

    fn cards(&self) -> Cards {
        self.possible_world_data_vec
            .first()
            .expect("never empty")
            .game_conclusion_for_plays
            .cards()
    }
}

// TODO: Requirement for observed_gameplay to have the observer active.
pub fn run_monte_carlo_simulation<R, W>(
    initial_state: &ObservedInitialGameState,
    observed_tricks: &ObservedPlayedCards,
    sample_size: usize,
    rng: &mut R,
    w: &mut W,
) -> Result<SampledWorldsData<YieldSoFar>, io::Error>
where
    R: Rng + ?Sized,
    W: io::Write,
{
    let card_knowledge = CardKnowledge::from_observation(initial_state, observed_tricks);

    let dist =
        UniformPossibleDealsFromObservedGameplay::new(&card_knowledge, initial_state.game_type());

    dbg!(dist.color_distributions());
    dbg!(dist.number_of_possibilities());

    let do_all_samples_threshold = 10_000.max(sample_size);
    let do_all_samples = dist.number_of_possibilities() <= do_all_samples_threshold;

    let sampled_deals = if do_all_samples {
        dist.get_all_possibilities()
    } else {
        dist.sample_iter(rng).take(sample_size).collect_vec()
    };

    writeln!(w, "sampling from {} possible deals", sampled_deals.len())?;

    let start_solve = Instant::now();

    let possible_world_data_vec = sampled_deals
        .par_iter()
        .map(|&possible_deal| {
            let OpenGameState {
                open_situation,
                yield_so_far,
                matadors,
            } = observed_tricks.to_open_game_state(
                possible_deal,
                initial_state.bidding_winner(),
                initial_state.game_type(),
            );

            debug_assert_eq!(
                open_situation.active_role(),
                initial_state
                    .bidding_role()
                    .to_role(initial_state.bidding_winner())
            );
            debug_assert_eq!(observed_tricks.active_role(), initial_state.bidding_role());

            let mut solver = OpenSituationSolver::new(
                FastOpenSituationSolverCache::new(open_situation_reachable_from_to_u32_key(
                    open_situation,
                )),
                initial_state.game_type(),
            );

            //TODO: final_declarer_yield_for_possible_plays analogue for game conclusions
            let game_conclusion_for_plays =
                final_declarer_yield_for_possible_plays(open_situation, yield_so_far, &mut solver);
            // .map(GameConclusion::from_final_declarer_yield);

            PossibleWorldData {
                game_conclusion_for_plays,
                matadors,
                probability_weight: 1.0,
            }
        })
        .collect();

    let end_solve = Instant::now();

    eprintln!(
        "multithreaded solving of {} sample deals took {:?}",
        sampled_deals.len(),
        end_solve - start_solve
    );

    Ok(SampledWorldsData {
        possible_world_data_vec,
    })
}

fn write_table_line(
    w: &mut impl io::Write,
    scores: AnalyzedPossiblePlays<f64>,
) -> Result<(), io::Error> {
    for card in scores.cards() {
        write!(w, " {:>6.2}", scores[&card])?;
    }

    writeln!(w)
}

fn write_table_header(w: &mut impl io::Write, cards: Cards) -> Result<(), io::Error> {
    for card in cards {
        write!(w, "     {:?}", card)?;
    }

    writeln!(w)
}

pub fn write_statistics(
    data: SampledWorldsData<YieldSoFar>,
    w: &mut impl io::Write,
) -> Result<(), io::Error> {
    let average_scores =
        data.weighted_average(|yield_so_far, _| yield_so_far.card_points().0 as f64);

    write_table_header(w, data.cards())?;
    write_table_line(w, average_scores)?;

    Ok(())
}
