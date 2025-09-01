use std::{
    fs::{File, read_to_string},
    io::{self, BufWriter, stdout},
    path::PathBuf,
};

use clap::{Parser, Subcommand};
use muskat::{analyze_observation::analyze_observations, dto::Dto, rng::cheap_rng};

#[derive(Parser)]
// #[command(version, about, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    File {
        path: PathBuf,
        iterations: usize,
        #[arg(short, long)]
        output: Option<PathBuf>,
        #[arg(short, long)]
        timing: Option<PathBuf>,
    },
}

fn main() -> Result<(), anyhow::Error> {
    println!("Hello, world!");

    let cli = Cli::parse();

    match cli.command {
        Commands::File {
            path,
            iterations,
            output,
            timing,
        } => {
            let mut w = match output {
                Some(path) => {
                    let outpup_file = File::create(path)?;
                    Some(BufWriter::new(outpup_file))
                }
                None => None,
            };

            let mut wt = match timing {
                Some(path) => {
                    let outpup_file = File::create(path)?;
                    Some(BufWriter::new(outpup_file))
                }
                None => None,
            };

            match (&mut w, &mut wt) {
                (None, None) => main_file(path, iterations, &mut stdout(), &mut stdout()),
                (None, Some(wt)) => main_file(path, iterations, &mut stdout(), wt),
                (Some(w), None) => main_file(path, iterations, w, &mut stdout()),
                (Some(w), Some(wt)) => main_file(path, iterations, w, wt),
            }
        }
    }
}

fn main_file(
    path: PathBuf,
    iterations: usize,
    w: &mut impl io::Write,
    wt: &mut impl io::Write,
) -> Result<(), anyhow::Error> {
    let dto: Dto = serde_json::from_str(&read_to_string(path)?)?;

    let mut rng = cheap_rng(3421);

    let initial = dto.pre_game_observations()?;
    let observed_turns = dto.played_cards();

    analyze_observations(&initial, observed_turns, iterations, &mut rng, w, wt)?;

    Ok(())
}
