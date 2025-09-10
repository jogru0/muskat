use std::ops::Mul;

use derive_more::{Add, AddAssign, Sum};
use static_assertions::assert_eq_size;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash, AddAssign, Add, Sum)]
pub struct CardPoints(pub u8);

assert_eq_size!(CardPoints, u8);

impl Mul<CardPoints> for u8 {
    type Output = CardPoints;

    fn mul(self, rhs: CardPoints) -> Self::Output {
        CardPoints(self * rhs.0)
    }
}
