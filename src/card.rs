use crate::{card_points::CardPoints, cards::Cards, game_type::GameType};
use serde::Deserialize;
use static_assertions::assert_eq_size;
use strum::{EnumCount, VariantArray};

#[derive(Clone, Copy, PartialEq, Eq, Deserialize, Hash)]
// Ordered for U
#[repr(u8)]
pub enum Suit {
    Diamonds,
    #[serde(alias = "Herz")]
    Hearts,
    #[serde(alias = "Green")]
    Spades,
    #[serde(alias = "Eichel")]
    Clubs,
}

assert_eq_size!(Suit, u8);

#[derive(Clone, Copy, PartialEq, Eq, Deserialize, Hash)]
pub enum CardType {
    #[serde(alias = "Grand")]
    Trump,
    #[serde(untagged)]
    Suit(Suit),
}
impl CardType {
    pub(crate) fn base_value(&self) -> i16 {
        match self {
            CardType::Trump => 24,
            CardType::Suit(Suit::Clubs) => 12,
            CardType::Suit(Suit::Spades) => 11,
            CardType::Suit(Suit::Hearts) => 10,
            CardType::Suit(Suit::Diamonds) => 9,
        }
    }
}

assert_eq_size!(CardType, u8);

#[derive(Clone, Copy)]
// Ordered for Null and partially for Nonnull.
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
    SO,
    SK,
    SZ,
    SA,
    H7,
    H8,
    H9,
    HO,
    HK,
    HZ,
    HA,
    G7,
    G8,
    G9,
    GO,
    GK,
    GZ,
    GA,
    E7,
    E8,
    E9,
    EO,
    EK,
    EZ,
    EA,
    SU,
    HU,
    GU,
    EU,
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
        match self {
            Card::S7
            | Card::S8
            | Card::S9
            | Card::SO
            | Card::SK
            | Card::SZ
            | Card::SU
            | Card::SA => Suit::Diamonds,
            Card::H7
            | Card::H8
            | Card::H9
            | Card::HO
            | Card::HK
            | Card::HZ
            | Card::HU
            | Card::HA => Suit::Hearts,
            Card::G7
            | Card::G8
            | Card::G9
            | Card::GO
            | Card::GK
            | Card::GZ
            | Card::GU
            | Card::GA => Suit::Spades,
            Card::E7
            | Card::E8
            | Card::E9
            | Card::EO
            | Card::EK
            | Card::EZ
            | Card::EU
            | Card::EA => Suit::Clubs,
        }
    }

    pub const fn rank(self) -> Rank {
        match self {
            Card::S7 | Card::H7 | Card::G7 | Card::E7 => Rank::L7,
            Card::S8 | Card::H8 | Card::G8 | Card::E8 => Rank::L8,
            Card::S9 | Card::H9 | Card::G9 | Card::E9 => Rank::L9,
            Card::SZ | Card::HZ | Card::GZ | Card::EZ => Rank::Z,
            Card::SU | Card::HU | Card::GU | Card::EU => Rank::U,
            Card::SO | Card::HO | Card::GO | Card::EO => Rank::O,
            Card::SK | Card::HK | Card::GK | Card::EK => Rank::K,
            Card::SA | Card::HA | Card::GA | Card::EA => Rank::A,
        }
    }

    pub const fn card_type(self, game_type: GameType) -> CardType {
        let trump = Cards::of_trump(game_type);

        if trump.contains(self) {
            return CardType::Trump;
        }

        CardType::Suit(self.suit())
    }

    pub fn of(rank: Rank, suit: Suit) -> Self {
        match (rank, suit) {
            (Rank::L7, Suit::Diamonds) => Card::S7,
            (Rank::L7, Suit::Hearts) => Card::H7,
            (Rank::L7, Suit::Spades) => Card::G7,
            (Rank::L7, Suit::Clubs) => Card::E7,
            (Rank::L8, Suit::Diamonds) => Card::S8,
            (Rank::L8, Suit::Hearts) => Card::H8,
            (Rank::L8, Suit::Spades) => Card::G8,
            (Rank::L8, Suit::Clubs) => Card::E8,
            (Rank::L9, Suit::Diamonds) => Card::S9,
            (Rank::L9, Suit::Hearts) => Card::H9,
            (Rank::L9, Suit::Spades) => Card::G9,
            (Rank::L9, Suit::Clubs) => Card::E9,
            (Rank::Z, Suit::Diamonds) => Card::SZ,
            (Rank::Z, Suit::Hearts) => Card::HZ,
            (Rank::Z, Suit::Spades) => Card::GZ,
            (Rank::Z, Suit::Clubs) => Card::EZ,
            (Rank::U, Suit::Diamonds) => Card::SU,
            (Rank::U, Suit::Hearts) => Card::HU,
            (Rank::U, Suit::Spades) => Card::GU,
            (Rank::U, Suit::Clubs) => Card::EU,
            (Rank::O, Suit::Diamonds) => Card::SO,
            (Rank::O, Suit::Hearts) => Card::HO,
            (Rank::O, Suit::Spades) => Card::GO,
            (Rank::O, Suit::Clubs) => Card::EO,
            (Rank::K, Suit::Diamonds) => Card::SK,
            (Rank::K, Suit::Hearts) => Card::HK,
            (Rank::K, Suit::Spades) => Card::GK,
            (Rank::K, Suit::Clubs) => Card::EK,
            (Rank::A, Suit::Diamonds) => Card::SA,
            (Rank::A, Suit::Hearts) => Card::HA,
            (Rank::A, Suit::Spades) => Card::GA,
            (Rank::A, Suit::Clubs) => Card::EA,
        }
    }
}
