use crate::analyze_observation::GameMode;
use crate::card::Card;
use crate::observed_gameplay::ObservedInitialGameState;
use crate::{bidding_role::BiddingRole, cards::Cards};
use serde::Deserialize;

#[derive(Deserialize)]
pub struct Dto {
    //TODO pub
    pub position: BiddingRole,
    pub hand: Cards,
    pub skat: Cards,
    pub game_mode: GameMode,
    // bidding_value: usize,
    played_cards: Vec<Vec<Card>>,
}
impl Dto {
    pub fn pre_game_observations(&self) -> ObservedInitialGameState {
        let skat_if_known = match self.skat.len() {
            0 => None,
            2 => Some(self.skat),
            _ => panic!("data makes no sense"),
        };

        ObservedInitialGameState::new(
            self.hand,
            skat_if_known,
            self.game_mode.game_type(),
            self.position,
            self.game_mode.bidding_winner(),
        )
    }

    pub fn played_cards(&self) -> impl Iterator<Item = Card> {
        self.played_cards.iter().flatten().copied()
    }
}
