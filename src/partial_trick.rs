use static_assertions::assert_eq_size;

use crate::{card::Card, cards::Cards, trick::Trick};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PartialTrick(PartialTrickImpl);

assert_eq_size!(PartialTrick, u16);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum PartialTrickImpl {
    Empty,
    OneCard(Card),
    TwoCards { first: Card, second: Card },
}

impl PartialTrick {
    pub const EMPTY: Self = Self(PartialTrickImpl::Empty);

    pub fn first(self) -> Option<Card> {
        match self.0 {
            PartialTrickImpl::Empty => None,
            PartialTrickImpl::OneCard(card) => Some(card),
            PartialTrickImpl::TwoCards { first, second: _ } => Some(first),
        }
    }

    pub fn second(self) -> Option<Card> {
        match self.0 {
            PartialTrickImpl::Empty | PartialTrickImpl::OneCard(_) => None,
            PartialTrickImpl::TwoCards { first: _, second } => Some(second),
        }
    }

    pub fn is_in_progress(self) -> bool {
        match self.0 {
            PartialTrickImpl::Empty => false,
            PartialTrickImpl::OneCard(_)
            | PartialTrickImpl::TwoCards {
                first: _,
                second: _,
            } => true,
        }
    }

    pub(crate) fn add(&mut self, card: Card) -> Option<Trick> {
        match self.0 {
            PartialTrickImpl::Empty => {
                self.0 = PartialTrickImpl::OneCard(card);
                None
            }
            PartialTrickImpl::OneCard(first) => {
                debug_assert_ne!(first, card);
                self.0 = PartialTrickImpl::TwoCards {
                    first,
                    second: card,
                };
                None
            }
            PartialTrickImpl::TwoCards { first, second } => {
                // Trick::new does the `debug_assert` for us.
                self.0 = PartialTrickImpl::Empty;
                Some(Trick::new(first, second, card))
            }
        }
    }

    pub fn cards(self) -> Cards {
        match self.0 {
            PartialTrickImpl::Empty => Cards::EMPTY,
            PartialTrickImpl::OneCard(card) => Cards::just(card),
            PartialTrickImpl::TwoCards { first, second } => {
                Cards::just(first).or(Cards::just(second))
            }
        }
    }

    pub fn number_of_cards(self) -> u8 {
        match self.0 {
            PartialTrickImpl::Empty => 0,
            PartialTrickImpl::OneCard(_) => 1,
            PartialTrickImpl::TwoCards {
                first: _,
                second: _,
            } => 2,
        }
    }
}
