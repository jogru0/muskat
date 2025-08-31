use std::mem::transmute;

use serde::Deserialize;
use static_assertions::assert_eq_size;
use strum::{EnumCount, VariantArray};

use crate::{card_points::CardPoints, cards::Cards, game_type::GameType};

#[derive(Clone, Copy, PartialEq, Eq, Deserialize)]
#[repr(u8)]
pub enum Suit {
    Diamonds,
    Hearts,
    Spades,
    #[serde(alias = "Eichel")]
    Clubs,
}

assert_eq_size!(Suit, u8);

#[derive(Clone, Copy, PartialEq, Eq, Deserialize)]
pub enum CardType {
    Trump,
    #[serde(untagged)]
    Suit(Suit),
}

assert_eq_size!(CardType, u8);

#[derive(Clone, Copy)]
#[repr(u8)]
pub enum Rank {
    L7,
    L8,
    L9,
    Z,
    U,
    O,
    K,
    A,
}

assert_eq_size!(Rank, u8);

impl Rank {
    const fn to_points(self) -> CardPoints {
        match self {
            Rank::L7 | Rank::L8 | Rank::L9 => CardPoints(0),
            Rank::Z => CardPoints(10),
            Rank::U => CardPoints(2),
            Rank::O => CardPoints(3),
            Rank::K => CardPoints(4),
            Rank::A => CardPoints(11),
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug, VariantArray, EnumCount, Deserialize, Hash)]
#[repr(u8)]
pub enum Card {
    S7,
    S8,
    S9,
    SZ,
    SU,
    SO,
    SK,
    SA,
    H7,
    H8,
    H9,
    HZ,
    HU,
    HO,
    HK,
    HA,
    G7,
    G8,
    G9,
    GZ,
    GU,
    GO,
    GK,
    GA,
    E7,
    E8,
    E9,
    EZ,
    EU,
    EO,
    EK,
    EA,
}

assert_eq_size!(Card, u8);

impl Card {
    pub const fn to_u8(self) -> u8 {
        self as u8
    }

    pub(crate) const fn detail_to_bit(self) -> u32 {
        1 << self.to_u8()
    }

    pub const fn to_points(self) -> CardPoints {
        self.rank().to_points()
    }

    pub const fn suit(self) -> Suit {
        let suite_id = self.to_u8() / 8;
        debug_assert!(suite_id < 4);
        unsafe { transmute(suite_id) }
    }

    pub const fn rank(self) -> Rank {
        let rank_id = self.to_u8() % 8;
        debug_assert!(rank_id < 8);
        unsafe { transmute(rank_id) }
    }

    pub const fn card_type(self, game_type: GameType) -> CardType {
        let trump = Cards::of_trump(game_type);

        if trump.contains(self) {
            return CardType::Trump;
        }

        CardType::Suit(self.suit())
    }
}
