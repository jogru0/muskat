use crate::card::CardType;
use serde::Deserialize;
use static_assertions::assert_eq_size;

#[derive(Clone, Copy, Deserialize, Hash, Debug)]
pub enum GameType {
    Null,
    #[serde(untagged)]
    Trump(CardType),
}

assert_eq_size!(GameType, u8);
