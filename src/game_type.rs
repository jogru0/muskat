use crate::card::CardType;
use rand::Rng;
use serde::Deserialize;
use static_assertions::assert_eq_size;

#[derive(Clone, Copy, Deserialize, Hash, Debug)]
pub enum GameType {
    Null,
    #[serde(untagged)]
    Trump(CardType),
}

impl GameType {
    pub fn random(rng: &mut impl Rng) -> Self {
        if rng.random() {
            Self::Null
        } else {
            Self::Trump(CardType::random(rng))
        }
    }
}

assert_eq_size!(GameType, u8);
