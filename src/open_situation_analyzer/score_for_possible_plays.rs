use crate::{
    open_situation_analyzer::{AnalyzedPossiblePlays, analyze_all_possible_plays},
    open_situation_solver::{OpenSituationSolver, bounds_cache::OpenSituationSolverCache},
    situation::OpenSituation,
    trick_yield::YieldSoFar,
};

//How many points will still follow for the declarer due to finishing tricks?
//Not counting tricks already won, or points gedrückt.
//TODO: Null schould be handled more elegantly.
pub fn final_declarer_yield_for_possible_plays<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    yield_so_far: YieldSoFar,
    open_situation_solver: &mut OpenSituationSolver<C>,
) -> AnalyzedPossiblePlays<YieldSoFar> {
    analyze_all_possible_plays(
        open_situation,
        open_situation_solver.game_type(),
        yield_so_far,
        |child, mut yield_so_far_child| {
            let future_yield_child = open_situation_solver
                .calculate_future_yield_with_optimal_open_play(child, yield_so_far_child);
            yield_so_far_child.add_assign(future_yield_child);
            yield_so_far_child
        },
    )
}

// TODO: mod name
// TODO: Not for Null!
// TODO: Only for new situation.
pub fn initial_situation_makes_at_least<C: OpenSituationSolverCache>(
    open_situation: OpenSituation,
    open_situation_solver: &mut OpenSituationSolver<C>,
    to_reach: YieldSoFar,
) -> bool {
    let delta = to_reach.saturating_sub(open_situation.yield_from_skat());
    open_situation_solver.still_makes_at_least(open_situation, delta)
}
