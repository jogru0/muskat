use crate::card::Card;
use crate::game_type::GameType;
use crate::monte_carlo::{Announcement, AnnouncementError, Contract};
use crate::observed_gameplay::ObservedInitialGameState;
use crate::{bidding_role::BiddingRole, cards::Cards};
use serde::Deserialize;

//TODO: Struct makes sense like this?
#[derive(Deserialize, Clone, Copy)]
struct AnnouncementDto {
    #[serde(alias = "declarer")]
    bidding_winner: BiddingRole,
    #[serde(alias = "type")]
    game_type: GameType,
    #[serde(default)]
    hand: bool,
    #[serde(default)]
    schneider: bool,
    #[serde(default)]
    schwarz: bool,
    #[serde(default)]
    ouvert: bool,
}

impl AnnouncementDto {
    fn game_type(&self) -> GameType {
        self.game_type
    }

    fn bidding_winner(&self) -> BiddingRole {
        self.bidding_winner
    }

    fn announcement(&self) -> Result<Announcement, AnnouncementError> {
        Announcement::new(
            self.game_type,
            self.hand,
            self.schneider,
            self.schwarz,
            self.ouvert,
        )
    }
}

#[derive(Deserialize)]
pub struct Dto {
    position: BiddingRole,
    hand: Cards,
    skat: Cards,
    game_mode: AnnouncementDto,
    bidding_value: i16,
    played_cards: Vec<Vec<Card>>,
}
impl Dto {
    pub fn pre_game_observations(&self) -> Result<ObservedInitialGameState, AnnouncementError> {
        let skat_if_known = match self.skat.len() {
            0 => None,
            2 => Some(self.skat),
            _ => panic!("data makes no sense"),
        };

        let contract = Contract::new(self.bidding_value, self.game_mode.announcement()?);

        Ok(ObservedInitialGameState::new(
            self.hand,
            skat_if_known,
            self.game_mode.game_type(),
            self.position,
            self.game_mode.bidding_winner(),
            contract,
        ))
    }

    pub fn played_cards(&self) -> impl Iterator<Item = Card> {
        self.played_cards.iter().flatten().copied()
    }
}
