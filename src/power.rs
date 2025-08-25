use crate::{
    card::{Card, Rank},
    game_type::GameType,
};
use static_assertions::assert_eq_size;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
pub struct CardPower(i8);

impl CardPower {
    // Used exactly for cards not having their normal power since they din't match the first card in the trick.
    // Has to be the smallest possible power.
    const DEACTIVATED: Self = Self(-1);

    pub fn comes_from_deactivated_card(self) -> bool {
        self == Self::DEACTIVATED
    }

    pub fn of(card: Card, first_card_in_trick: Card, game_type: GameType) -> Self {
        let card_is_trump = game_type.trump_cards().contains(card);
        let card_follows_trick_type = first_card_in_trick
            .cards_following_suite(game_type)
            .contains(card);

        if !card_is_trump && !card_follows_trick_type {
            return CardPower::DEACTIVATED;
        }

        let rank = card.rank();

        let result = CardPower(match game_type {
            GameType::Null => rank as i8,
            GameType::Suit(..) | GameType::Grand => {
                let mut result = match rank {
                    Rank::Z => 7,
                    Rank::A => 8,
                    Rank::U => 9 + card.suit() as i8,
                    other_rank => other_rank as i8,
                };

                if card_is_trump {
                    //TODO: Why not 9?
                    result += 10;
                }
                result
            }
        });

        debug_assert!(CardPower::DEACTIVATED < result);
        result
    }
}

assert_eq_size!(CardPower, u8);
