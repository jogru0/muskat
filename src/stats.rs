use itertools::Itertools;
use std::io;

pub fn write_stats(
    name: &str,
    data: impl Iterator<Item = f64>,
    wt: &mut impl io::Write,
) -> Result<(), io::Error> {
    writeln!(wt, "{name}:")?;

    let mut v = data.collect_vec();
    debug_assert!(!v.is_empty());

    v.sort_by(f64::total_cmp);

    let mean = v.iter().sum::<f64>() / v.len() as f64;

    let median = if v.len() % 2 == 0 {
        v[v.len() / 2].midpoint(v[v.len() / 2 - 1])
    } else {
        v[v.len() / 2]
    };

    let max = v.last().expect("not empty");

    writeln!(wt, "\tmean: {mean:.0}")?;
    writeln!(wt, "\tmedian: {median:.0}")?;
    writeln!(wt, "\tmax: {max:.0}")
}
