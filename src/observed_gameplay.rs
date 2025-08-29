use serde::Deserialize;

use crate::{
    bidding_role::BiddingRole, cards::Cards, deal::Deal, dist::UnknownCards, game_type::GameType,
    partial_trick::PartialTrick, position::Position, trick::Trick,
};

pub struct ObservedGameplay {
    done_tricks: Vec<Trick>,
    current_trick: PartialTrick,
    start_hand: Cards,
    skat_if_known: Option<Cards>,
    game_type: GameType,
    bidding_role: BiddingRole,
}

#[derive(Deserialize)]
pub struct GameModeDto {
    declarer: String,
    r#type: String,
}

#[derive(Deserialize)]
pub struct Dto {
    position: String,
    hand: Cards,
    skat: Cards,
    game_mode: GameModeDto,
    // bidding_value: u8,
    // played_cards: Vec<Trick>,
}

impl ObservedGameplay {
    pub fn new(
        start_hand: Cards,
        skat_if_known: Option<Cards>,
        game_type: GameType,
        bidding_role: BiddingRole,
    ) -> Self {
        Self {
            done_tricks: Vec::new(),
            current_trick: PartialTrick::EMPTY,
            start_hand,
            skat_if_known,
            game_type,
            bidding_role,
        }
    }

    pub fn game_type(&self) -> GameType {
        self.game_type
    }

    fn unknown_cards_and_observed_cards_before_tricks(&self) -> ([UnknownCards; 4], Deal) {
        let skat = if self.skat_if_known.is_some() {
            UnknownCards::ALL_KNOWN
        } else {
            UnknownCards::UNKNOWN_SKAT
        };

        let mut unknown_cards = [
            UnknownCards::UNKNOWN_HAND,
            UnknownCards::UNKNOWN_HAND,
            UnknownCards::UNKNOWN_HAND,
            skat,
        ];

        let mut deal = Deal::new(
            Cards::EMPTY,
            Cards::EMPTY,
            Cards::EMPTY,
            self.skat_if_known.unwrap_or(Cards::EMPTY),
        );

        unknown_cards[Self::bidding_role_id(self.bidding_role)] = UnknownCards::ALL_KNOWN;
        *deal.hand_mut(self.bidding_role) = self.start_hand;

        (unknown_cards, deal)
    }

    fn bidding_role_id(bidding_role: BiddingRole) -> usize {
        match bidding_role {
            BiddingRole::FirstReceiver => 0,
            BiddingRole::FirstCaller => 1,
            BiddingRole::SecondCaller => 2,
        }
    }

    pub fn unknown_cards_vec_and_observed_cards(&self) -> ([UnknownCards; 4], Deal) {
        let (mut unknown_cards, mut deal) = self.unknown_cards_and_observed_cards_before_tricks();

        let mut first_player = BiddingRole::FIRST_ACTIVE_PLAYER;

        for trick in &self.done_tricks {
            let second_player = first_player.next();
            let third_player = second_player.next();

            let first_card = trick.first();
            let first_type = first_card.card_type(self.game_type);

            if self.bidding_role != first_player {
                unknown_cards[Self::bidding_role_id(first_player)].remove(1);
                deal.hand_mut(first_player).add_new(first_card);
            }

            if self.bidding_role != second_player {
                let second_card = trick.second();
                let second_type = second_card.card_type(self.game_type);
                let second_hand = &mut unknown_cards[Self::bidding_role_id(second_player)];

                second_hand.remove(1);
                deal.hand_mut(second_player).add_new(second_card);

                if second_type != first_type {
                    second_hand.retrict(first_type);
                }
            }

            if self.bidding_role != third_player {
                let third_card = trick.third();
                let third_type = third_card.card_type(self.game_type);
                let third_hand = &mut unknown_cards[Self::bidding_role_id(third_player)];

                third_hand.remove(1);
                deal.hand_mut(third_player).add_new(third_card);

                if third_type != first_type {
                    third_hand.retrict(first_type);
                }
            }

            match trick.winner_position(self.game_type) {
                Position::Forehand => {}
                Position::Middlehand => first_player = first_player.next(),
                Position::Rearhand => first_player = first_player.next().next(),
            }
        }

        if let Some(first_card) = self.current_trick.first() {
            let first_type = first_card.card_type(self.game_type);
            if self.bidding_role != first_player {
                unknown_cards[Self::bidding_role_id(first_player)].remove(1);
                deal.hand_mut(first_player).add_new(first_card);
            }

            let second_player = first_player.next();
            if let Some(second_card) = self.current_trick.second()
                && self.bidding_role != second_player
            {
                let second_type = second_card.card_type(self.game_type);
                let second_hand = &mut unknown_cards[Self::bidding_role_id(second_player)];

                second_hand.remove(1);
                deal.hand_mut(second_player).add_new(second_card);

                if second_type != first_type {
                    second_hand.retrict(first_type);
                }
            }
        }

        (unknown_cards, deal)
    }
}

#[cfg(test)]
mod tests {
    use std::fs::File;

    use anyhow::Ok;

    use crate::{
        bidding_role::BiddingRole,
        card::{CardType, Suit},
        dist::UniformPossibleOpenSituationsFromObservedGameplay,
        game_type::GameType,
        observed_gameplay::{Dto, ObservedGameplay},
    };

    #[test]
    fn test_deserialize_sl003() -> Result<(), anyhow::Error> {
        let result: Dto = serde_json::from_reader(File::open("res/sl003.json")?)?;

        let observed_gameplay = ObservedGameplay::new(
            result.hand,
            Some(result.skat),
            GameType::Trump(CardType::Suit(Suit::Clubs)),
            BiddingRole::SecondCaller,
        );

        let dist = UniformPossibleOpenSituationsFromObservedGameplay::new(
            observed_gameplay,
            BiddingRole::FirstCaller,
        );

        assert_eq!(dist.color_distributions(), 140);
        assert_eq!(dist.possible_situations(), 184756);

        Ok(())
    }
}
