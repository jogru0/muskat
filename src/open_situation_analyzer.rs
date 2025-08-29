use crate::{game_type::GameType, situation::OpenSituation, trick_yield::YieldSoFar};

pub mod score_for_possible_plays;

pub struct AnalyzedPossiblePlays<R>([Option<R>; 32]);

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
    let mut result = [const { None }; 32];

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
        result[card.to_u8() as usize] = Some(analyzer(child, yield_so_far_child));
    }

    AnalyzedPossiblePlays(result)
}
