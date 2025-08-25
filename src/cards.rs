use static_assertions::assert_eq_size;
use std::{fmt::Debug, mem::transmute};
use strum::VariantArray;

use crate::{
    card::{Card, Rank, Suit},
    card_points::CardPoints,
    game_type::GameType,
};

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Cards {
    bits: u32,
}

assert_eq_size!(Cards, u32);

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

const fn suit_id(suit: Suit) -> u8 {
    match suit {
        Suit::Diamonds => 0,
        Suit::Hearts => 1,
        Suit::Spades => 2,
        Suit::Clubs => 3,
    }
}

const fn rank_id(rank: Rank) -> u8 {
    match rank {
        Rank::L7 => 0,
        Rank::L8 => 1,
        Rank::L9 => 2,
        Rank::Z => 3,
        Rank::U => 4,
        Rank::O => 5,
        Rank::K => 6,
        Rank::A => 7,
    }
}

impl Cards {
    pub const EMPTY: Self = Self { bits: 0 };

    pub const ALL: Self = Self { bits: u32::MAX };

    const fn via_suit_id(suit_id: u8) -> Self {
        assert!(suit_id < 4);
        Self {
            bits: 0b00000000_00000000_00000000_11111111 << (8 * suit_id),
        }
    }

    const fn via_rank_id(rank_id: u8) -> Self {
        assert!(rank_id < 8);
        Self {
            bits: 0b00000001_00000001_00000001_00000001 << rank_id,
        }
    }

    pub const fn just(card: Card) -> Self {
        Self {
            bits: card.detail_to_bit(),
        }
    }

    pub const fn of_suit(suit: Suit) -> Self {
        Self::via_suit_id(suit_id(suit))
    }

    pub const fn of_rank(rank: Rank) -> Self {
        Self::via_rank_id(rank_id(rank))
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

        let self_and_following_suit = self.and(first_trick_card.cards_following_suite(game_type));

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

    pub const fn remove_next(&mut self) -> Card {
        debug_assert!(!self.is_empty());
        let id_card = self.bits.trailing_zeros() as u8;
        debug_assert!(id_card < 32);

        //TODO: Speed compared to just call remove?
        self.bits ^= 1 << id_card;

        unsafe { transmute(id_card) }
    }

    //TODO: Fast?
    pub fn to_points(self) -> CardPoints {
        let mut result = CardPoints(0);
        for i in 0..32 {
            let card = unsafe { transmute::<u8, Card>(i) };
            if self.contains(card) {
                result = CardPoints(result.0 + card.to_points().0);
            }
        }
        result
    }

    pub fn add(&mut self, card: Card) {
        debug_assert!(!self.contains(card));
        self.bits |= card.detail_to_bit()
    }

    pub(crate) fn detail_to_bits(self) -> u32 {
        self.bits
    }
}
