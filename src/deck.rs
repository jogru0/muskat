use rand::{Rng, seq::SliceRandom};
use static_assertions::const_assert_eq;
use strum::{EnumCount, VariantArray};

use crate::card::Card;
use std::{ops::Index, slice::SliceIndex};

pub struct Deck([Card; 32]);

impl Deck {
    fn unshuffled() -> Self {
        const_assert_eq!(Card::COUNT, 32);
        let result = Card::VARIANTS.try_into();
        // TODO: Thanks, I hate it. Get the enum boilerplare sorted out.
        let array = unsafe { result.unwrap_unchecked() };
        Self(array)
    }

    pub fn shuffled(rng: &mut impl Rng) -> Self {
        let mut result = Self::unshuffled();
        result.0.shuffle(rng);
        result
    }
}

impl<I> Index<I> for Deck
where
    I: SliceIndex<[Card]>,
{
    type Output = I::Output;

    fn index(&self, index: I) -> &Self::Output {
        self.0.index(index)
    }
}
