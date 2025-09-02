use std::{fs::File, io::BufWriter, io::Write};

use itertools::Itertools;
use muskat::{
    open_analysis_performance::{
        Level, generate_random_trump_games_of_active_player,
        measure_performance_to_decide_winner_of_open_situations,
        measure_performance_to_judge_possible_next_turns_of_open_situation,
    },
    rng::cheap_rng,
};

#[test]
#[ignore = "performance"]
pub fn test_performance_2003_is_win() -> Result<(), anyhow::Error> {
    let output_file = File::create("res/performance_2003.log")?;
    let mut wt = BufWriter::new(output_file);

    if cfg!(debug_assertions) {
        writeln!(
            wt,
            "[🚨 🚨 🚨] please disable debug assertions for performance measurements [🚨 🚨 🚨]"
        )?;
    }

    let mut rng = cheap_rng(432);

    let fixed_setup = generate_random_trump_games_of_active_player(&mut rng)
        .take(100)
        .collect_vec();

    let performance_results = measure_performance_to_decide_winner_of_open_situations(
        fixed_setup.iter().copied(),
        Level::IsWon,
    );
    performance_results.write(&mut wt)?;

    let performance_results = measure_performance_to_decide_winner_of_open_situations(
        fixed_setup.iter().copied(),
        Level::Conclusion,
    );
    performance_results.write(&mut wt)?;

    let performance_results = measure_performance_to_decide_winner_of_open_situations(
        fixed_setup.iter().copied(),
        Level::Yield,
    );
    performance_results.write(&mut wt)?;

    writeln!(wt, "\n//////////\n")?;

    let performance_results = measure_performance_to_judge_possible_next_turns_of_open_situation(
        fixed_setup.iter().copied(),
        Level::IsWon,
    );
    performance_results.write(&mut wt)?;

    let performance_results = measure_performance_to_judge_possible_next_turns_of_open_situation(
        fixed_setup.iter().copied(),
        Level::Conclusion,
    );
    performance_results.write(&mut wt)?;

    let performance_results = measure_performance_to_judge_possible_next_turns_of_open_situation(
        fixed_setup.iter().copied(),
        Level::Yield,
    );
    performance_results.write(&mut wt)?;

    Ok(())
}
