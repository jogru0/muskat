use static_assertions::assert_eq_size;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash)]
pub struct CardPoints(pub u8);

assert_eq_size!(CardPoints, u8);
