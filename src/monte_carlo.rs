use crate::{
    card::{Card, CardType},
    dist::UniformPossibleDealsFromObservedGameplay,
    game_type::GameType,
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
    fmt::Debug,
    io::{self},
    time::Instant,
};
use thiserror::Error;

#[derive(PartialEq, Eq, PartialOrd, Ord, Clone, Copy, Debug)]
pub enum GameConclusion {
    DeclarerIsSchwarz,
    DeclarerIsSchneider,
    DeclarerIsDominated,
    DefendersAreDominated,
    DefendersAreSchneider,
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
            (90..=118, 1..=9) => Self::DefendersAreSchneider,
            (_, 10) => Self::DefendersAreSchwarz,
            _ => panic!("invalid: {final_declarer_yield:?}"),
        }
    }

    fn someone_is_schneider(&self) -> bool {
        match self {
            GameConclusion::DeclarerIsDominated | GameConclusion::DefendersAreDominated => false,
            GameConclusion::DefendersAreSchneider
            | GameConclusion::DeclarerIsSchwarz
            | GameConclusion::DeclarerIsSchneider
            | GameConclusion::DefendersAreSchwarz => true,
        }
    }

    fn someone_is_schwarz(&self) -> bool {
        match self {
            GameConclusion::DeclarerIsSchneider
            | GameConclusion::DefendersAreSchneider
            | GameConclusion::DeclarerIsDominated
            | GameConclusion::DefendersAreDominated => false,
            GameConclusion::DeclarerIsSchwarz | GameConclusion::DefendersAreSchwarz => true,
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

impl<A> PossibleWorldData<A> {
    pub fn map<F, B>(&self, f: F) -> PossibleWorldData<B>
    where
        F: Fn(&A) -> B,
    {
        PossibleWorldData {
            game_conclusion_for_plays: self.game_conclusion_for_plays.map(f),
            matadors: self.matadors,
            probability_weight: self.probability_weight,
        }
    }

    pub fn plays(&self) -> &AnalyzedPossiblePlays<A> {
        &self.game_conclusion_for_plays
    }
}

#[derive(Debug)]
pub struct SampledWorldsData<A> {
    // Never empty. Consistend possible plays.
    possible_world_data_vec: Vec<PossibleWorldData<A>>,
}

impl<A: PartialEq> SampledWorldsData<A> {
    pub fn to_unique(&self) -> Option<&AnalyzedPossiblePlays<A>> {
        let result = self
            .possible_world_data_vec
            .first()
            .expect("not empty")
            .plays();

        for other in &self.possible_world_data_vec[1..] {
            if other.plays() != result {
                return None;
            }
        }

        Some(result)
    }
}

impl<A> SampledWorldsData<A> {
    pub fn map<F, B>(&self, f: F) -> SampledWorldsData<B>
    where
        F: Fn(&A) -> B,
    {
        SampledWorldsData {
            possible_world_data_vec: self
                .possible_world_data_vec
                .iter()
                .map(|data| data.map(&f))
                .collect(),
        }
    }

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

    fn map_to_card(&self) -> AnalyzedPossiblePlays<Card> {
        self.possible_world_data_vec
            .first()
            .expect("never empty")
            .game_conclusion_for_plays
            .map_to_card()
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

fn write_table_line_numbers(
    w: &mut impl io::Write,
    name: &str,
    scores: AnalyzedPossiblePlays<f64>,
) -> Result<(), io::Error> {
    write!(w, "{name:4} |")?;
    for card in scores.cards() {
        write!(w, " {:>6.2}", scores[&card])?;
    }
    writeln!(w)
}

fn write_table_line_debug<D: Debug>(
    w: &mut impl io::Write,
    name: &str,
    scores: &AnalyzedPossiblePlays<D>,
) -> Result<(), io::Error> {
    write!(w, "{name:4} |")?;
    for card in scores.cards() {
        write!(w, " {:>6}", format!("{:?}", scores[&card]))?;
    }
    writeln!(w)
}

fn write_table_line_propabilities_if_interesting(
    w: &mut impl io::Write,
    name: &str,
    probabilities: AnalyzedPossiblePlays<f64>,
) -> Result<(), io::Error> {
    if probabilities.are_all(&1.0) || probabilities.are_all(&0.0) {
        return Ok(());
    }

    write!(w, "{name:4} |")?;
    for card in probabilities.cards() {
        write!(w, " {:>5.1}%", 100.0 * probabilities[&card])?;
    }
    writeln!(w)
}

fn write_table_header(
    w: &mut impl io::Write,
    cards: &AnalyzedPossiblePlays<Card>,
) -> Result<(), io::Error> {
    write_table_line_debug(w, "card", cards)
}

pub fn write_statistics(
    declarer_yield_data: SampledWorldsData<YieldSoFar>,
    contract: Contract,
    w: &mut impl io::Write,
) -> Result<(), io::Error> {
    let average_card_points =
        declarer_yield_data.weighted_average(|yield_so_far, _| yield_so_far.card_points().0 as f64);

    let game_conclusion_data = declarer_yield_data.map(|final_declarer_yield| {
        GameConclusion::from_final_declarer_yield(final_declarer_yield)
    });

    write_table_header(w, &declarer_yield_data.map_to_card())?;
    write_table_line_numbers(w, "avg.", average_card_points)?;

    if let Some(unique_results) = game_conclusion_data.to_unique() {
        // TODO: Correct formatting
        write_table_line_debug(w, "res.", unique_results)?;
    } else {
        // TODO: Swaps for Null?
        let probabilities_not_lost_schwarz =
            game_conclusion_data.weighted_probability_of(|&game_conclusion, _| {
                GameConclusion::DeclarerIsSchwarz < game_conclusion
            });

        let probabilities_not_lost_schneider =
            game_conclusion_data.weighted_probability_of(|&game_conclusion, _| {
                GameConclusion::DeclarerIsSchneider < game_conclusion
            });

        let probabilities_won =
            game_conclusion_data.weighted_probability_of(|&game_conclusion, _| {
                GameConclusion::DeclarerIsDominated < game_conclusion
            });

        let probabilities_won_schneider =
            game_conclusion_data.weighted_probability_of(|&game_conclusion, _| {
                GameConclusion::DefendersAreDominated < game_conclusion
            });

        let probabilities_won_schwarz =
            game_conclusion_data.weighted_probability_of(|&game_conclusion, _| {
                GameConclusion::DefendersAreSchneider < game_conclusion
            });

        write_table_line_propabilities_if_interesting(w, "nlb", probabilities_not_lost_schwarz)?;
        write_table_line_propabilities_if_interesting(w, "nls", probabilities_not_lost_schneider)?;
        write_table_line_propabilities_if_interesting(w, "w", probabilities_won)?;
        write_table_line_propabilities_if_interesting(w, "ws", probabilities_won_schneider)?;
        write_table_line_propabilities_if_interesting(w, "wb", probabilities_won_schwarz)?;
    }

    let average_classical_score_delta =
        game_conclusion_data.weighted_average(|game_conclusion, maybe_matador| {
            contract
                .conclude(game_conclusion, maybe_matador)
                .score_classical()
                .delta() as f64
        });

    write_table_line_numbers(w, "game", average_classical_score_delta)?;

    Ok(())
}

#[derive(Clone, Copy)]
pub struct Contract {
    bidding_value: i16,
    announcement: Announcement,
}

fn value_due_to_overbid(overbid: i16, game_type: GameType) -> i16 {
    let base_value = match game_type {
        GameType::Null => {
            // TODO: Implement this.
            // In that case, I think, the result is the smallest value
            // a contract of a trump game would have had. (ISkO 2022 3.6.2)
            // I think, this is calculated without schneider or schwarz multipliers ...
            // Note that in order to calculate that, we need the matadors for
            // all possible trump game announcements ...
            unimplemented!("come on, don't overbid null!")
        }
        GameType::Trump(card_type) => card_type.base_value(),
    };

    (overbid + base_value - 1) / base_value
}

impl Contract {
    pub fn conclude(self, conclusion: &GameConclusion, matadors: Option<u8>) -> GameResult {
        let announcement_value = self.announcement.value(conclusion, matadors);

        if announcement_value < self.bidding_value {
            // Oh no, overbid.
            let value_due_to_overbid =
                value_due_to_overbid(self.bidding_value, self.announcement.game_type());

            GameResult::new(value_due_to_overbid, false)
        } else {
            GameResult::new(announcement_value, self.announcement.is_won(conclusion))
        }
    }
}

// TODO: Explain what was wrong
#[derive(Error, Debug)]
pub enum AnnouncementError {
    #[error("invalid announcement")]
    Invalid,
}

impl Announcement {
    pub fn new(
        game_type: GameType,
        hand: bool,
        schneider: bool,
        schwarz: bool,
        ouvert: bool,
    ) -> Result<Self, AnnouncementError> {
        let result = match game_type {
            GameType::Null => {
                if schneider || schwarz {
                    return Err(AnnouncementError::Invalid);
                };
                AnnouncementImpl::Null { hand, ouvert }
            }
            GameType::Trump(card_type) => {
                let level = match (hand, schneider, schwarz, ouvert) {
                    (true, true, true, true) => TrumpGameLevel::Ouvert,
                    (true, true, true, false) => TrumpGameLevel::Schwarz,
                    (true, true, false, true) => TrumpGameLevel::Schneider,
                    (true, false, false, false) => TrumpGameLevel::Hand,
                    (false, false, false, false) => TrumpGameLevel::Normal,
                    _ => return Err(AnnouncementError::Invalid),
                };

                AnnouncementImpl::Trump {
                    trump: card_type,
                    level,
                }
            }
        };

        Ok(Self(result))
    }

    /// What is the value of this announcement, given a certain game conclusion?
    /// This is the value that can be bid without overbidding.
    /// Notably, this is always positive. The game conclusion does influence it only insofar
    /// that it checks if someone is schneider or schwarz, irrespectible of who.
    pub fn value(&self, conclusion: &GameConclusion, matadors: Option<u8>) -> i16 {
        let (trump, level, matadors) = match (self.0, matadors) {
            (AnnouncementImpl::Null { hand, ouvert }, None) => {
                return match (hand, ouvert) {
                    (true, true) => 59,
                    (true, false) => 35,
                    (false, true) => 46,
                    (false, false) => 23,
                };
            }
            (AnnouncementImpl::Trump { trump, level }, Some(matadors)) => {
                (trump, level, matadors as i16)
            }
            _ => unreachable!("game type doesnt match existence of matadors"),
        };

        let base_value = trump.base_value();

        // Mit/Ohne n ...
        let mut multiplier = matadors;

        // ... Spiel n + 1 ...
        multiplier += 1;

        if TrumpGameLevel::Hand <= level {
            // ... Hand n + 2 ...
            multiplier += 1;
        }

        if conclusion.someone_is_schneider() || TrumpGameLevel::Schneider <= level {
            // ... Schneider x ...
            multiplier += 1;
        }

        if TrumpGameLevel::Schneider <= level {
            // ... angesagt x + 1 ...
            multiplier += 1;
        }

        if conclusion.someone_is_schwarz() || TrumpGameLevel::Schwarz <= level {
            // ... Schwarz y ...
            multiplier += 1;
        }

        if TrumpGameLevel::Schwarz <= level {
            // ... angesagt y + 1 ...
            multiplier += 1;
        }

        if matches!(level, TrumpGameLevel::Ouvert) {
            // ... Ouvert y + 2 ...
            multiplier += 1;
        }

        multiplier * base_value
    }

    fn game_type(&self) -> GameType {
        match self.0 {
            AnnouncementImpl::Null { hand: _, ouvert: _ } => GameType::Null,
            AnnouncementImpl::Trump { trump, level: _ } => GameType::Trump(trump),
        }
    }

    fn is_won(&self, conclusion: &GameConclusion) -> bool {
        match self.0 {
            AnnouncementImpl::Null { hand: _, ouvert: _ } => {
                matches!(conclusion, GameConclusion::DeclarerIsSchwarz)
            }
            AnnouncementImpl::Trump { trump: _, level } => {
                &level.conclusion_to_reach() <= conclusion
            }
        }
    }
}

#[derive(Clone, Copy)]
pub struct Announcement(AnnouncementImpl);

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
enum TrumpGameLevel {
    Normal,
    Hand,
    Schneider,
    Schwarz,
    Ouvert,
}
impl TrumpGameLevel {
    fn conclusion_to_reach(&self) -> GameConclusion {
        match self {
            Self::Normal | Self::Hand => GameConclusion::DefendersAreDominated,
            Self::Schneider => GameConclusion::DefendersAreSchneider,
            Self::Schwarz | Self::Ouvert => GameConclusion::DefendersAreSchwarz,
        }
    }
}

#[derive(Clone, Copy)]
enum AnnouncementImpl {
    Null {
        hand: bool,
        ouvert: bool,
    },
    Trump {
        trump: CardType,
        level: TrumpGameLevel,
    },
}

impl Contract {
    pub(crate) fn new(bidding_value: i16, announcement: Announcement) -> Self {
        Self {
            bidding_value,
            announcement,
        }
    }
}

pub struct GameResult {
    /// What the game is worth according to the rules. Always positive.
    /// If the game was overbid, or ended schneider or schwarz (even if not announced as such),
    /// that is already reflected by an increased value here.
    game_value: i16,
    /// If the game was won by the declarer.
    /// Might be falso due to unfulfilled announcments (e.g. announced schwarz, just reacched schndeider),
    /// or due to overbidding.
    is_won: bool,
}

/// How the scores of the players at the table are impacted by the result of a game.
struct ScoreImpact {
    on_declarer: i16,
    // includes defender, but also players of the table that were not active this game.
    on_others: i16,
}
impl ScoreImpact {
    fn delta(&self) -> i16 {
        self.on_declarer - self.on_others
    }
}

impl GameResult {
    fn score_classical(&self) -> ScoreImpact {
        if self.is_won {
            ScoreImpact {
                on_declarer: self.game_value,
                on_others: 0,
            }
        } else {
            ScoreImpact {
                on_declarer: -2 * self.game_value,
                on_others: 0,
            }
        }
    }

    pub fn new(game_value: i16, is_won: bool) -> Self {
        Self { game_value, is_won }
    }
}
