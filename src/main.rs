use std::{fs::read_to_string, io::stdout, path::PathBuf};

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
    File { path: PathBuf, iterations: usize },
}

fn main() -> Result<(), anyhow::Error> {
    println!("Hello, world!");

    let cli = Cli::parse();

    match cli.command {
        Commands::File { path, iterations } => {
            let dto: Dto = serde_json::from_str(&read_to_string(path)?)?;

            let mut rng = cheap_rng(3421);

            let initial = dto.pre_game_observations();
            let observed_turns = dto.played_cards();

            analyze_observations(
                &initial,
                observed_turns,
                iterations,
                &mut rng,
                &mut stdout(),
            )?;
        }
    }

    Ok(())
}
