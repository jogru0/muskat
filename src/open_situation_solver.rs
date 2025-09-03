use crate::bounds::Bounds;
use crate::card::Card;
use crate::card_points::CardPoints;
use crate::cards::{Cards, quasi_equivalent_with_max_delta};
use crate::game_type::GameType;
use crate::minimax_role::MinimaxRole;
use crate::open_situation_solver::bounds_and_preference::BoundsAndMaybePreference;
use crate::open_situation_solver::bounds_cache::OpenSituationSolverCache;
use crate::power::CardPower;
use crate::situation::OpenSituation;
use crate::trick_yield::{TrickYield, YieldSoFar};
use std::cmp::Reverse;
use std::time::{Duration, Instant};

mod bounds_and_preference;

/// Cards to consider for playing next, to be checked in that order.
/// Result is only None after first None.
///
/// `open_situation` should still be ongoing.
fn get_cards_to_consider(
    open_situation: OpenSituation,
    game_type: GameType,
    maybe_preference: Option<Card>,
) -> [Option<Card>; 10] {
    let mut result = [None; 10];

    let mut cards = open_situation.next_possible_plays(game_type);
    debug_assert!(!cards.is_empty());
    debug_assert!(cards.len() < 11);

    let mut next_index = 0;
    if let Some(preference) = maybe_preference {
        result[next_index] = Some(preference);
        cards.remove(preference);
        next_index += 1;
    }

    let maybe_fixed_first_trick_card = open_situation.maybe_first_trick_card();

    let n = open_situation.active_role().next();
    let nn = n.next();
    let n_h = open_situation.hand_cards_of(n);
    let nn_h = open_situation.hand_cards_of(nn);

    // TODO: Why for every card individually, it only depends on the suit w.r.t. the game type.
    let get_options = |card| {
        if maybe_fixed_first_trick_card.is_some() {
            // We don't need to calculate as it doesn't depend on `card` anyway.
            return 0;
        }

        n_h.possible_plays(Some(card), game_type).len()
            * nn_h.possible_plays(Some(card), game_type).len()
    };

    //TODO: First doesn't need to be a copy.
    //TODO: Can't we store the values, or even sort?
    while !cards.is_empty() {
        let mut cards_copy = cards;
        let mut best_card = unsafe { cards_copy.remove_next_unchecked() };

        // lower is better
        let mut best_options = get_options(best_card);

        //higher is better
        let mut best_card_power = CardPower::of(
            best_card,
            //TODO: Is this calculation for free or should we buffer?
            maybe_fixed_first_trick_card
                .unwrap_or(best_card)
                .card_type(game_type),
            game_type,
        );

        while let Some(card) = cards_copy.remove_next() {
            let options = get_options(card);
            let card_power = CardPower::of(
                card,
                maybe_fixed_first_trick_card
                    .unwrap_or(card)
                    .card_type(game_type),
                game_type,
            );

            if (options, Reverse(card_power)) < (best_options, Reverse(best_card_power)) {
                best_card = card;
                best_options = options;
                best_card_power = card_power;
            }
        }

        result[next_index] = Some(best_card);
        cards.remove(best_card);
        next_index += 1;
    }

    result
}

pub mod bounds_cache;

pub struct OpenSituationSolver<C> {
    cache: C,
    game_type: GameType,
    time_spend_calculating_bounds: Duration,
    analyzed_nodes: usize,
}

impl<C: OpenSituationSolverCache> OpenSituationSolver<C> {
    /// # Note
    ///
    /// This, together with `bounds_deciding_threshold`, is the tight hot recursive loop
    /// at the core of the `OpenSituationSolver`. Therefore, these two together are
    /// highly optimized and should be changes with care.
    ///
    /// `open_situation` should still be ongoing.
    // TODO: Performance for splitting this in maximizing and minimizing.
    fn improve_bounds_to_decide_threshold(
        &mut self,
        mut bounds: Bounds,
        maybe_preference: Option<Card>,
        open_situation: OpenSituation,
        threshold: TrickYield,
    ) -> BoundsAndMaybePreference {
        let active_minimax_role = open_situation.active_minimax_role(self.game_type);

        //TODO: Performance not checking one level higher?
        debug_assert!(!bounds.decides_threshold(threshold));

        let mut bound_calculated_over_all_children = TrickYield::worst(active_minimax_role);

        let cards_to_consider =
            get_cards_to_consider(open_situation, self.game_type, maybe_preference);

        //We catch these via strict bounds.
        debug_assert!(cards_to_consider[0].is_some());

        let mut maybe_deciding_card = None;

        let mut banned_due_to_being_too_quasi_equivalent = Cards::EMPTY;

        let in_hand_or_yielded = open_situation.in_hand_or_yielded();

        // TODO: function that returns maybe_deciding_card?
        for maybe_card in cards_to_consider {
            let Some(card) = maybe_card else {
                break;
            };

            if banned_due_to_being_too_quasi_equivalent.contains(card) {
                continue;
            }

            let mut child = open_situation;

            //TODO: Does it make sense to return Option<CardPoints> here?
            let additional_score = child.play_card(card, self.game_type);
            let threshold_child = threshold.saturating_sub(additional_score);

            let bounds_child = self.bounds_deciding_threshold(child, threshold_child);

            //TODO: `add_assign` threshold to bounds.
            //TODO: Do we want add_assign or add?
            let mut lower_bound_via_child = bounds_child.lower();
            lower_bound_via_child.add_assign(additional_score);

            let mut upper_bound_via_child = bounds_child.upper();
            upper_bound_via_child.add_assign(additional_score);

            debug_assert!(
                Bounds::new(lower_bound_via_child, upper_bound_via_child)
                    .decides_threshold(threshold)
            );

            //TODO: was constexpr
            match active_minimax_role {
                MinimaxRole::Min => {
                    bounds.minimize_upper(upper_bound_via_child);
                }
                MinimaxRole::Max => {
                    bounds.maximize_lower(lower_bound_via_child);
                }
            }

            if bounds.decides_threshold(threshold) {
                maybe_deciding_card = Some(card);
                break;
            }

            let delta = Bounds::new(lower_bound_via_child, upper_bound_via_child)
                .distance_to_threshold(threshold);
            let (banned, delta) = quasi_equivalent_with_max_delta(
                card,
                in_hand_or_yielded,
                self.game_type,
                delta.min(CardPoints(1)),
            );

            //TODO: was constexpr
            match active_minimax_role {
                MinimaxRole::Min => {
                    bound_calculated_over_all_children =
                        bound_calculated_over_all_children.min(lower_bound_via_child.sub(delta));
                }
                MinimaxRole::Max => {
                    bound_calculated_over_all_children =
                        bound_calculated_over_all_children.max(upper_bound_via_child.add(delta));
                }
            }

            banned_due_to_being_too_quasi_equivalent =
                banned_due_to_being_too_quasi_equivalent.or(banned)
        }

        // TODO: Understand again what this case means and why we are sure it decides the threshold, but no card is suggested for play.
        if maybe_deciding_card.is_none() {
            // TODO: was constexpr
            match active_minimax_role {
                MinimaxRole::Min => {
                    // All childs win.
                    bounds.update_lower(bound_calculated_over_all_children);
                }
                MinimaxRole::Max => {
                    // No winning child.
                    bounds.update_upper(bound_calculated_over_all_children);
                }
            }

            // TODO: Test again. Also, would it make sense to prefer the best effort?
            // maybe_deciding_card = pref;
        }

        debug_assert!(bounds.decides_threshold(threshold));
        BoundsAndMaybePreference::new(bounds, maybe_deciding_card)
    }

    /// Finds and caches bounds of the situation that show if the declarer
    /// can reach the treshold (lower bound at least threshold)
    /// or not (upper bound below threshold).
    ///
    /// # Note
    ///
    /// This, together with `improve_bounds_to_decide_threshold`, is the tight hot recursive loop
    /// at the core of the `OpenSituationSolver`. Therefore, these two together are
    /// highly optimized and should be changes with care.
    fn bounds_deciding_threshold(
        &mut self,
        open_situation: OpenSituation,
        threshold: TrickYield,
    ) -> Bounds {
        self.analyzed_nodes += 1;

        // We try to avoid using the cache for situations without multiple parents.
        // Specifically, we don't cache situations with an ongoing trick.
        if !open_situation.is_trick_in_progress() {
            let mut bounds_and_preference = self.cache.get_copy(open_situation);
            if !bounds_and_preference.bounds().decides_threshold(threshold) {
                bounds_and_preference = self.improve_bounds_to_decide_threshold(
                    bounds_and_preference.bounds(),
                    bounds_and_preference.maybe_preference(),
                    open_situation,
                    threshold,
                );

                debug_assert!(bounds_and_preference.bounds().decides_threshold(threshold));

                self.cache
                    .update_existing(open_situation, bounds_and_preference);
            }

            return bounds_and_preference.bounds();
        }

        let mut bounds = open_situation.quick_bounds();
        if !bounds.decides_threshold(threshold) {
            bounds = self
                .improve_bounds_to_decide_threshold(bounds, None, open_situation, threshold)
                .bounds();
            debug_assert!(bounds.decides_threshold(threshold));
        }

        bounds
    }

    pub fn new(cache: C, game_type: GameType) -> Self {
        Self {
            cache,
            game_type,
            time_spend_calculating_bounds: Duration::ZERO,
            analyzed_nodes: 0,
        }
    }

    // TODO: Should probably not be a member of this struct.
    // TODO: I think for Null, it just returns any non empty yield in case of the declarer losing.
    // So this makes only sense to be called with completely new games? Or is there some outside logic
    // making sure that we calculate the total yield correctly?
    // If this is meant to abort when it's clear that we can't reach the target, could that be used for non null as well?
    pub fn calculate_future_yield_with_optimal_open_play(
        &mut self,
        open_situation: OpenSituation,
        yield_so_far: YieldSoFar,
    ) -> TrickYield {
        if matches!(self.game_type, GameType::Null) {
            // TODO: Should we check whether score so far is NONE?
            if self.still_makes_at_least(open_situation, TrickYield::new(CardPoints(0), 1)) {
                return TrickYield::new(CardPoints(0), 1);
            }
            return TrickYield::ZERO_TRICKS;
        }

        // TODO: Duplication to call this here, should we instead just use MAX and then let the cache work for us?
        let quick_bounds = open_situation.quick_bounds();

        let needed_for_schwarz = YieldSoFar::MAX.saturating_sub(yield_so_far);
        debug_assert!(quick_bounds.upper() <= needed_for_schwarz);

        if needed_for_schwarz == quick_bounds.upper() {
            //Try to force Schwarz for the declarer.
            if self.still_makes_at_least(open_situation, needed_for_schwarz) {
                return needed_for_schwarz;
            }
        }
        debug_assert!(!self.still_makes_at_least(open_situation, needed_for_schwarz));

        if quick_bounds.upper().number_of_tricks() == 0 {
            // We already know that no tricks can be made.
            debug_assert_eq!(quick_bounds.upper().card_points().0, 0);
            debug_assert!(
                !self.still_makes_at_least(open_situation, TrickYield::new(CardPoints(0), 1))
            );
            debug_assert_eq!(quick_bounds.lower(), TrickYield::ZERO_TRICKS);
            return TrickYield::ZERO_TRICKS;
        }

        //See if we make points (and therefore one trick), and how many, or no points, but at least one trick.
        //TODO: E.g. here, it is not necessary to check if we can make at least one trick,
        // when the yield_so_far is not NONE anyway.
        let mut goal = TrickYield::new(quick_bounds.upper().card_points(), 1);
        while !self.still_makes_at_least(open_situation, goal) {
            if goal.card_points().0 == 0 {
                //Looks like the declarer cannot make a single trick.
                debug_assert_eq!(quick_bounds.lower(), TrickYield::ZERO_TRICKS);
                return TrickYield::ZERO_TRICKS;
            }
            goal = TrickYield::new(CardPoints(goal.card_points().0 - 1), 1);
        }

        //TODO: Would be really cool if we could return the actual threshold we found, not
        //the arbitrary one with 1 trick.
        debug_assert!(quick_bounds.lower() <= goal);
        goal
    }

    /// Main entry point for algorithms using the solver.
    pub fn still_makes_at_least(
        &mut self,
        open_situation: OpenSituation,
        threshold: TrickYield,
    ) -> bool {
        let start_time = Instant::now();
        let bounds = self.bounds_deciding_threshold(open_situation, threshold);
        let end_time = Instant::now();
        self.time_spend_calculating_bounds += end_time - start_time;

        if threshold <= bounds.lower() {
            return true;
        }

        // TODO: Kinda logic duplication with bounds.decides_threshold
        debug_assert!(bounds.upper() < threshold);
        false
    }

    pub fn game_type(&self) -> GameType {
        self.game_type
    }

    pub fn nodes_generated(&self) -> usize {
        self.analyzed_nodes
    }

    pub fn time_spent(&self) -> Duration {
        self.time_spend_calculating_bounds
    }
}

#[cfg(test)]
mod tests;
