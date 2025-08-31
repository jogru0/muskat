use std::io::{self, Write};

use rand::Rng;
use serde::Deserialize;

use crate::{
    bidding_role::BiddingRole,
    card::Card,
    game_type::GameType,
    monte_carlo::{run_monte_carlo_simulation, write_statistics},
    observed_gameplay::{ObservedInitialGameState, ObservedPlayedCards},
};

//TODO: Struct makes sense like this?
#[derive(Deserialize, Clone, Copy)]
pub struct GameMode {
    #[serde(alias = "declarer")]
    bidding_winner: BiddingRole,
    #[serde(alias = "type")]
    game_type: GameType,
}
impl GameMode {
    pub fn game_type(&self) -> GameType {
        self.game_type
    }

    pub fn bidding_winner(&self) -> BiddingRole {
        self.bidding_winner
    }
}

fn analyze_decision(
    initial_state: &ObservedInitialGameState,
    observed_tricks: &ObservedPlayedCards,
    sample_size: usize,
    rng: &mut (impl Rng + ?Sized),
    w: &mut impl Write,
) -> Result<(), io::Error> {
    let data = run_monte_carlo_simulation(initial_state, observed_tricks, sample_size, rng, w)?;
    write_statistics(data, w)
}

pub fn analyze_observations(
    initial: &ObservedInitialGameState,
    mut turns: impl Iterator<Item = Card>,
    sample_size: usize,
    rng: &mut (impl Rng + ?Sized),
    w: &mut impl Write,
) -> Result<(), io::Error> {
    let mut observed_tricks = ObservedPlayedCards::initial();
    loop {
        if observed_tricks.active_role() == initial.bidding_role() {
            analyze_decision(initial, &observed_tricks, sample_size, rng, w)?;
        }

        let Some(next_card) = turns.next() else {
            return writeln!(w, "Analysis done.");
        };

        observed_tricks.observe_play(next_card, initial.game_type());
        writeln!(w, "Played: {:?}", next_card)?;
    }
}
