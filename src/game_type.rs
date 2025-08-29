use crate::card::CardType;
use static_assertions::assert_eq_size;

#[derive(Clone, Copy)]
pub enum GameType {
    Trump(CardType),
    Null,
}

assert_eq_size!(GameType, u8);
