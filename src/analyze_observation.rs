use crate::{
    card::Card,
    monte_carlo::{run_monte_carlo_simulation, write_statistics},
    observed_gameplay::{ObservedInitialGameState, ObservedPlayedCards},
};
use rand::Rng;
use std::io::{self, Write};

fn analyze_options(
    initial_state: &ObservedInitialGameState,
    observed_tricks: &ObservedPlayedCards,
    sample_size: usize,
    rng: &mut (impl Rng + ?Sized),
    w: &mut impl Write,
) -> Result<(), io::Error> {
    let data = run_monte_carlo_simulation(initial_state, observed_tricks, sample_size, rng, w)?;
    write_statistics(data, initial_state.contract(), w)
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
            analyze_options(initial, &observed_tricks, sample_size, rng, w)?;
        }

        let Some(next_card) = turns.next() else {
            return writeln!(w, "Analysis done.");
        };

        observed_tricks.observe_play(next_card, initial.game_type());
        writeln!(w, "Played: {:?}", next_card)?;
    }
}
