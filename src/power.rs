use crate::{
    card::{Card, CardType, Rank},
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

    pub fn of(card: Card, trick_type: CardType, game_type: GameType) -> Self {
        let card_type = card.card_type(game_type);

        if !matches!(card_type, CardType::Trump) && card_type != trick_type {
            return CardPower::DEACTIVATED;
        }

        let rank = card.rank();

        let result = CardPower(match game_type {
            GameType::Null => rank as i8,
            GameType::Trump(_) => {
                let mut result = match rank {
                    Rank::Z => 7,
                    Rank::A => 8,
                    Rank::U => 9 + card.suit() as i8,
                    other_rank => other_rank as i8,
                };

                if matches!(card_type, CardType::Trump) {
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
