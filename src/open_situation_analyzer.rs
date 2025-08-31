use std::{collections::HashMap, iter::Sum, ops::Index};

use crate::{
    card::Card, cards::Cards, game_type::GameType, situation::OpenSituation,
    trick_yield::YieldSoFar,
};

pub mod score_for_possible_plays;

#[derive(Debug, PartialEq, Eq)]
pub struct AnalyzedPossiblePlays<R>(HashMap<Card, R>);

impl Sum for AnalyzedPossiblePlays<f64> {
    fn sum<I: Iterator<Item = Self>>(mut iter: I) -> Self {
        let mut result = iter.next().expect("no empty sum possible");

        for next in iter {
            debug_assert_eq!(result.0.len(), next.0.len());
            for (k, v) in &mut result.0 {
                *v += next.0[k];
            }
        }

        result
    }
}

impl<R> Index<&Card> for AnalyzedPossiblePlays<R> {
    type Output = R;

    fn index(&self, index: &Card) -> &Self::Output {
        self.0.index(index)
    }
}

impl<R: PartialEq> AnalyzedPossiblePlays<R> {
    pub fn are_all(&self, val: &R) -> bool {
        self.0.iter().all(|(_, v)| v == val)
    }
}

impl<R> AnalyzedPossiblePlays<R> {
    pub fn map<F, Q>(&self, f: F) -> AnalyzedPossiblePlays<Q>
    where
        F: Fn(&R) -> Q,
    {
        AnalyzedPossiblePlays(self.0.iter().map(|(&k, v)| (k, f(v))).collect())
    }

    pub fn cards(&self) -> Cards {
        self.0.keys().collect()
    }

    pub fn map_to_card(&self) -> AnalyzedPossiblePlays<Card> {
        AnalyzedPossiblePlays(self.0.iter().map(|(&k, _)| (k, k)).collect())
    }

    pub fn new() -> Self {
        Self(HashMap::new())
    }

    pub fn add_new(&mut self, card: Card, value: R) {
        let old = self.0.insert(card, value);
        debug_assert!(old.is_none());
    }
}

impl<R> Default for AnalyzedPossiblePlays<R> {
    fn default() -> Self {
        Self::new()
    }
}

//How many points will still follow for the declarer due to finishing tricks?
//Not counting tricks already won, or points gedrückt.
//TODO: Null schould be handled more elegantly.
pub fn analyze_all_possible_plays<R, A>(
    open_situation: OpenSituation,
    //TODO: Implicitly already there in Analyzer?
    game_type: GameType,
    yield_so_far: YieldSoFar,
    mut analyzer: A,
) -> AnalyzedPossiblePlays<R>
where
    A: FnMut(OpenSituation, YieldSoFar) -> R,
{
    let mut result = AnalyzedPossiblePlays::new();

    let mut possible_plays = open_situation.next_possible_plays(game_type);
    debug_assert!(!possible_plays.is_empty());

    //TODO: Going over all childs should be its own thing.
    while let Some(card) = possible_plays.remove_next() {
        let mut child = open_situation;
        let points_to_get_to_child = child.play_card(card, game_type);
        let mut yield_so_far_child = yield_so_far;
        yield_so_far_child.add_assign(points_to_get_to_child);
        //TODO: For null, it makes no sense to see if the child results in subschwarz if points are obtained to get there.
        //TODO: It also makes no sense to look for schwarz results that are impossible due to points_to_get_to_child.

        //TODO: For the equivalent of the cpp way to analyze, make sure that the result
        // contains points_to_get_to_child (and probaly also yield_so_far, which is added later in the cpp version)
        result.add_new(card, analyzer(child, yield_so_far_child));
    }

    result
}
