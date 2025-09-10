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
        self.remove_smallest()
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

    pub const OF_ZERO_POINTS: Self = Self::of_rank(Rank::L7)
        .or(Self::of_rank(Rank::L8))
        .or(Self::of_rank(Rank::L9));

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

    pub fn remove_existing(&mut self, card: Card) {
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

    pub const fn remove_smallest(&mut self) -> Option<Card> {
        if self.is_empty() {
            return None;
        }

        Some(unsafe { self.remove_smallest_unchecked() })
    }

    /// # Safety
    ///
    /// self must not be empty.
    pub const unsafe fn remove_smallest_unchecked(&mut self) -> Card {
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

        while let Some(card) = self.remove_smallest() {
            result.push(card);
        }

        result
    }

    pub fn remove_highest_non_null(&mut self) -> Option<Card> {
        let id_card = self.bits.checked_ilog2()? as u8;
        debug_assert!(id_card < 32);
        self.bits ^= 1 << id_card;

        Some(unsafe { transmute::<u8, Card>(id_card) })
    }

    pub fn highest_non_null(mut self) -> Option<Card> {
        self.remove_highest_non_null()
    }

    pub fn remove_if_there(&mut self, card: Card) -> bool {
        let new = self.without(Cards::just(card));
        let was_there = &new != self;
        *self = new;
        was_there
    }

    pub fn remove_lowest_of_cards(&mut self, cards: Cards) -> Option<Card> {
        let card = self.and(cards).remove_smallest()?;
        self.remove_existing(card);
        Some(card)
    }

    pub fn remove_highest_of_cards(&mut self, cards: Cards) -> Option<Card> {
        let card = self.and(cards).remove_highest_non_null()?;
        self.remove_existing(card);
        Some(card)
    }

    pub fn remove_lowest_of_type(
        &mut self,
        card_type: CardType,
        game_type: GameType,
    ) -> Option<Card> {
        self.remove_lowest_of_cards(Cards::of_card_type(card_type, game_type))
    }

    /// If multiple have the same value, use the lowest of these.
    fn remove_highest_value(&mut self) -> Card {
        for rank in Rank::BY_POINTS.into_iter().rev() {
            if let Some(card) = self.and(Cards::of_rank(rank)).remove_smallest() {
                self.remove_existing(card);
                return card;
            }
        }

        unreachable!("empty cards")
    }

    /// If multiple have the same value, use the lowest of these.
    pub fn highest_value(mut self) -> Card {
        self.remove_highest_value()
    }

    // TODO: This order is weird. Maybe should just return points for n cards and not mutate to prevent misuse?
    pub fn remove_lowest_value(&mut self) -> Card {
        for rank in Rank::BY_POINTS {
            if let Some(card) = self.and(Cards::of_rank(rank)).remove_smallest() {
                self.remove_existing(card);
                return card;
            }
        }

        unreachable!("empty cards")
    }
}

/// Input the max delta that should lead to banning childs.
/// The returned CardPoints is the biggest delta of an actually banned card.
/// Therefore, we get as tight bounds as possible for banned childs to use
/// to tighten our own bounds.
pub fn quasi_equivalent_with_max_delta(
    card: Card,
    in_hand_or_yielded: Cards,
    game_type: GameType,
    max_delta: CardPoints,
    still_considered: Cards,
) -> (Cards, CardPoints) {
    if matches!(game_type, GameType::Null) {
        let sorted_ranks = [
            Rank::L7,
            Rank::L8,
            Rank::L9,
            Rank::Z,
            Rank::U,
            Rank::O,
            Rank::K,
            Rank::A,
        ];

        let suit = card.suit();

        // TODO: Think about null/schwarz. Do we ignore cases because we have a max delta, even though they would
        // saturate? I don't think so, but I am not sure.
        quasi_equivalent_impl(
            card,
            sorted_ranks.into_iter().map(|rank| Card::of(rank, suit)),
            in_hand_or_yielded,
            max_delta,
            still_considered,
        )
    } else {
        // We rely on the fact that for pure type Cards and not null games, iterating Cards is sorted by rank.
        quasi_equivalent_impl(
            card,
            Cards::of_card_type(card.card_type(game_type), game_type),
            in_hand_or_yielded,
            max_delta,
            still_considered,
        )
    }
}

fn quasi_equivalent_impl(
    card: Card,
    candidates_sorted_by_rank: impl IntoIterator<Item = Card>,
    in_hand_or_yielded: Cards,
    max_delta: CardPoints,
    still_considered: Cards,
) -> (Cards, CardPoints) {
    let mut filtered_equivalence_set = Cards::EMPTY;
    let mut result_max_delta = 0;
    let mut found_card = false;

    let card_points = card.to_points();

    for candidate in candidates_sorted_by_rank {
        if !in_hand_or_yielded.contains(candidate) {
            if found_card {
                break;
            }

            filtered_equivalence_set = Cards::EMPTY;
            result_max_delta = 0;
            continue;
        }

        found_card |= candidate == card;

        if !still_considered.contains(candidate) {
            continue;
        }

        let delta = candidate.to_points().0.abs_diff(card_points.0);
        if max_delta.0 < delta {
            continue;
        }

        filtered_equivalence_set.add_new(candidate);
        result_max_delta = result_max_delta.max(delta)
    }

    debug_assert!(found_card);
    (filtered_equivalence_set, CardPoints(result_max_delta))
}

impl<const N: usize> From<&[Card; N]> for Cards {
    fn from(cards: &[Card; N]) -> Self {
        let mut result = Self::EMPTY;
        for &card in cards {
            result.add_new(card);
        }
        result
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        card::{Card, CardType},
        card_points::CardPoints,
        cards::{Cards, quasi_equivalent_with_max_delta},
        game_type::GameType,
    };

    #[test]
    fn quasi_equivalence_u() {
        let card = Card::EU;
        let in_hand_or_yielded = Cards::from(&[Card::EU, Card::SU, Card::GU]);
        let game_type = GameType::Trump(CardType::Trump);
        let max_delta = CardPoints(0);
        let still_considered = Cards::from(&[Card::SU, Card::HU, Card::GU]);

        let (banned, delta) = quasi_equivalent_with_max_delta(
            card,
            in_hand_or_yielded,
            game_type,
            max_delta,
            still_considered,
        );

        assert_eq!(banned, Cards::from(&[Card::GU]));
        assert_eq!(delta, CardPoints(0));
    }
}
