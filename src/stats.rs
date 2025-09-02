use itertools::Itertools;
use std::{io, time::Duration};

pub fn write_node_timing_stats<'a, 'b>(
    nodes: impl IntoIterator<Item = &'a usize>,
    times: impl IntoIterator<Item = &'b Duration>,
    wt: &mut impl io::Write,
) -> Result<(), io::Error> {
    let mut nodes_of_1000 = nodes
        .into_iter()
        .map(|&node| node as f64 / 1000.)
        .collect_vec();

    let mut times_of_ms = times
        .into_iter()
        .map(|time| time.as_secs_f64() * 1000.)
        .collect_vec();

    debug_assert_eq!(times_of_ms.len(), nodes_of_1000.len());

    write_stats("Number of 1000 nodes", &mut nodes_of_1000, wt)?;
    write_stats("Time spend in ms", &mut times_of_ms, wt)?;

    let mean_time_of_s = calculate_mean(&mut times_of_ms) / 1000.;
    let mean_nodes = calculate_mean(&mut nodes_of_1000) * 1000.;

    let mean_duration_per_node = Duration::from_secs_f64(mean_time_of_s / mean_nodes);

    writeln!(wt, "Mean duration per node: {:?}", mean_duration_per_node)
}

fn calculate_mean(data: &mut [f64]) -> f64 {
    data.iter().sum::<f64>() / data.len() as f64
}

fn write_stats(name: &str, data: &mut [f64], wt: &mut impl io::Write) -> Result<(), io::Error> {
    writeln!(wt, "{name}:")?;

    debug_assert!(!data.is_empty());

    data.sort_unstable_by(f64::total_cmp);

    let mean = calculate_mean(data);

    let median = if data.len() % 2 == 0 {
        data[data.len() / 2].midpoint(data[data.len() / 2 - 1])
    } else {
        data[data.len() / 2]
    };

    let max = data.last().expect("not empty");

    writeln!(wt, "\tmean: {mean:.0}")?;
    writeln!(wt, "\tmedian: {median:.0}")?;
    writeln!(wt, "\tmax: {max:.0}")
}
