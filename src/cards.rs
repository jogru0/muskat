use serde::{Deserialize, de::Visitor};
use static_assertions::assert_eq_size;
use std::{fmt::Debug, mem::transmute};
use strum::VariantArray;

use crate::{
    card::{Card, CardType, Rank, Suit},
    card_points::CardPoints,
    game_type::GameType,
};

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct Cards {
    bits: u32,
}

assert_eq_size!(Cards, u32);

impl Iterator for Cards {
    type Item = Card;

    fn next(&mut self) -> Option<Self::Item> {
        self.remove_next()
    }
}

impl<'a> FromIterator<&'a Card> for Cards {
    fn from_iter<T: IntoIterator<Item = &'a Card>>(iter: T) -> Self {
        let mut result = Cards::EMPTY;
        for &card in iter {
            result.add_new(card);
        }
        result
    }
}

struct MySeqVisitor;

impl<'de> Visitor<'de> for MySeqVisitor {
    type Value = Cards;

    fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        formatter.write_str("a set of cards")
    }

    fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
    where
        A: serde::de::SeqAccess<'de>,
    {
        let mut result = Cards::EMPTY;

        while let Some(card) = seq.next_element()? {
            // TODO: Error handling
            result.add_new(card);
        }

        Ok(result)
    }
}

impl<'de> Deserialize<'de> for Cards {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        deserializer.deserialize_seq(MySeqVisitor)
    }
}

impl Debug for Cards {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut debug_set = f.debug_set();

        for &card in Card::VARIANTS {
            if self.contains(card) {
                debug_set.entry(&card);
            }
        }

        debug_set.finish()
    }
}

impl Cards {
    pub const EMPTY: Self = Self { bits: 0 };

    pub const ALL: Self = Self { bits: u32::MAX };

    pub const fn just(card: Card) -> Self {
        Self {
            bits: card.detail_to_bit(),
        }
    }

    pub const fn of_suit(suit: Suit) -> Self {
        let bits = match suit {
            Suit::Diamonds => 0b00010000_00000000_00000000_01111111,
            Suit::Hearts => 0b00100000_00000000_00111111_10000000,
            Suit::Spades => 0b01000000_00011111_11000000_00000000,
            Suit::Clubs => 0b10001111_11100000_00000000_00000000,
        };
        Self { bits }
    }

    pub const fn of_rank(rank: Rank) -> Self {
        let bits = match rank {
            Rank::L7 => 0b00000000_00100000_01000000_10000001,
            Rank::L8 => 0b00000000_01000000_10000001_00000010,
            Rank::L9 => 0b00000000_10000001_00000010_00000100,
            Rank::O => 0b00000001_00000010_00000100_00001000,
            Rank::K => 0b00000010_00000100_00001000_00010000,
            Rank::Z => 0b00000100_00001000_00010000_00100000,
            Rank::A => 0b00001000_00010000_00100000_01000000,
            Rank::U => 0b11110000_00000000_00000000_00000000,
        };
        Self { bits }
    }

    pub const fn of_trump(game_type: GameType) -> Self {
        match game_type {
            GameType::Trump(CardType::Suit(suit)) => Self::of_rank(Rank::U).or(Self::of_suit(suit)),
            GameType::Trump(CardType::Trump) => Self::of_rank(Rank::U),
            GameType::Null => Self::EMPTY,
        }
    }

    pub const fn of_card_type(card_type: CardType, game_type: GameType) -> Self {
        let trump = Cards::of_trump(game_type);

        match card_type {
            CardType::Trump => trump,
            CardType::Suit(suit) => Self::of_suit(suit).without(trump),
        }
    }

    pub const fn is_empty(self) -> bool {
        self.bits == 0
    }

    pub const fn len(self) -> u8 {
        self.bits.count_ones() as u8
    }

    pub const fn overlaps(self, other: Cards) -> bool {
        !self.and(other).is_empty()
    }

    pub const fn contains(self, card: Card) -> bool {
        self.overlaps(Cards::just(card))
    }

    pub fn remove(&mut self, card: Card) {
        debug_assert!(self.contains(card));
        *self = self.without(Cards::just(card))
    }

    /// Assumed to not be used on empty hand.
    ///
    /// Therefore guaranteed to return at least one option.
    pub const fn possible_plays(
        self,
        maybe_first_trick_card: Option<Card>,
        game_type: GameType,
    ) -> Cards {
        debug_assert!(!self.is_empty());

        let Some(first_trick_card) = maybe_first_trick_card else {
            return self;
        };

        let self_and_following_suit = self.and(Cards::of_card_type(
            first_trick_card.card_type(game_type),
            game_type,
        ));

        if self_and_following_suit.is_empty() {
            return self;
        }

        self_and_following_suit
    }

    pub const fn or(self, other: Cards) -> Cards {
        Cards {
            bits: self.bits | other.bits,
        }
    }

    pub const fn and(self, other: Cards) -> Cards {
        Cards {
            bits: self.bits & other.bits,
        }
    }

    pub const fn without(self, cards: Cards) -> Cards {
        Self {
            bits: self.bits & !cards.bits,
        }
    }

    pub const fn remove_next(&mut self) -> Option<Card> {
        if self.is_empty() {
            return None;
        }

        Some(unsafe { self.remove_next_unchecked() })
    }

    /// # Safety
    ///
    /// self must not be empty.
    pub const unsafe fn remove_next_unchecked(&mut self) -> Card {
        let id_card = self.bits.trailing_zeros() as u8;
        debug_assert!(id_card < 32);

        let bit_to_flip = 1 << id_card;
        debug_assert!(bit_to_flip == self.bits & (self.bits - 1) ^ self.bits);

        //TODO: Speed compared to just call remove?
        self.bits ^= bit_to_flip;

        unsafe { transmute(id_card) }
    }

    //TODO: Fast? Vs checking contains?
    pub fn to_points(self) -> CardPoints {
        let mut result = CardPoints(0);
        for card in self {
            result = CardPoints(result.0 + card.to_points().0);
        }
        result
    }

    pub const fn add_new(&mut self, card: Card) {
        debug_assert!(!self.contains(card));
        self.bits |= card.detail_to_bit()
    }

    pub(crate) fn detail_to_bits(self) -> u32 {
        self.bits
    }

    pub fn combined_with_disjoint(&self, other: Cards) -> Cards {
        assert!(self.and(other).is_empty());
        self.or(other)
    }

    pub fn to_vec(mut self) -> Vec<Card> {
        let mut result = Vec::new();

        while let Some(card) = self.remove_next() {
            result.push(card);
        }

        result
    }
}
