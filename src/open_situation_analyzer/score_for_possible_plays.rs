use crate::{
    open_situation_analyzer::{AnalyzedPossiblePlays, analyze_all_possible_plays},
    open_situation_solver::{OpenSituationSolver, bounds_cache::OpenSituationSolverCache},
    situation::OpenSituation,
    trick_yield::{TrickYield, YieldSoFar},
};

//How many points will still follow for the declarer due to finishing tricks?
//Not counting tricks already won, or points gedrückt.
//TODO: Null schould be handled more elegantly.
pub fn score_for_possible_plays<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    yield_so_far: YieldSoFar,
    open_situation_solver: &mut OpenSituationSolver<C>,
) -> AnalyzedPossiblePlays<TrickYield> {
    analyze_all_possible_plays(
        open_situation,
        open_situation_solver.game_type(),
        yield_so_far,
        |child, yield_so_far_child| {
            open_situation_solver
                .calculate_future_yield_with_optimal_open_play(child, yield_so_far_child)
        },
    )
}
