use crate::{
    card::{Rank, Suit},
    cards::Cards,
};
use static_assertions::assert_eq_size;

#[derive(Clone, Copy)]
pub enum GameType {
    Suit(Suit),
    Null,
    Grand,
}
impl GameType {
    pub const fn trump_cards(self) -> Cards {
        match self {
            GameType::Suit(suit) => Cards::of_suit(suit).or(Cards::of_rank(Rank::U)),
            GameType::Null => Cards::EMPTY,
            GameType::Grand => Cards::of_rank(Rank::U),
        }
    }
}

assert_eq_size!(GameType, u8);
