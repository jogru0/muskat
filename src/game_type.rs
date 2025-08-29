use crate::{
    card::{CardType, Rank},
    cards::Cards,
};
use static_assertions::assert_eq_size;

#[derive(Clone, Copy)]
pub enum GameType {
    Trump(CardType),
    Null,
}

assert_eq_size!(GameType, u8);

impl GameType {
    pub const fn trump_cards(self) -> Cards {
        match self {
            GameType::Trump(CardType::Suit(suit)) => {
                Cards::of_suit(suit).or(Cards::of_rank(Rank::U))
            }
            GameType::Null => Cards::EMPTY,
            GameType::Trump(CardType::Trump) => Cards::of_rank(Rank::U),
        }
    }
}
