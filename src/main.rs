use std::{
    fs::{File, read_to_string},
    io::stdout,
};

use muskat::{
    bidding_role::BiddingRole,
    card::{CardType, Suit},
    game_type::GameType,
    monte_carlo::{run_monte_carlo_simulation, write_statistics},
    observed_gameplay::{Dto, ObservedGameplay},
    rng::cheap_rng,
};

fn main() -> Result<(), anyhow::Error> {
    println!("Hello, world!");
    let input: Dto = serde_json::from_str(&read_to_string("res/sl003.json")?)?;

    let observed_gameplay = ObservedGameplay::new(
        input.hand,
        Some(input.skat),
        GameType::Trump(CardType::Suit(Suit::Clubs)),
        BiddingRole::FirstReceiver,
    );

    let rng = cheap_rng(432);

    let data = run_monte_carlo_simulation(observed_gameplay, BiddingRole::FirstReceiver, 1000, rng);

    write_statistics(&mut stdout(), data)?;

    Ok(())
}
