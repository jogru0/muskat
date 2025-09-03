use std::{
    fs::File,
    io::{BufWriter, Write},
};

use itertools::Itertools;
use muskat::{
    open_analysis_performance::{
        Level, generate_random_unfinished_open_situations,
        measure_performance_to_decide_winner_of_open_situations,
        measure_performance_to_judge_possible_next_turns_of_open_situation,
    },
    rng::cheap_rng,
};

#[test]
pub fn regression_open_solver() -> Result<(), anyhow::Error> {
    let output_file = File::create("res/regression_open_solver.log")?;
    let mut wt = BufWriter::new(output_file);

    let mut rng = cheap_rng(3231976);

    let fixed_setup = generate_random_unfinished_open_situations(&mut rng)
        .take(1000)
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
