use static_assertions::assert_eq_size;

use crate::{card_points::CardPoints, minimax_role::MinimaxRole, trick::Trick};

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
// Order of the member fields is important for deriving Ord.
// Ordering this way, we have for example:
// 28 card points with 2 tricks is better than 24 card point with 3 tricks.
pub struct TrickYield {
    card_points: CardPoints,
    number_of_tricks: u8,
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug, Hash)]
// Order of the member fields is important for deriving Ord.
// Ordering this way, we have for example:
// 28 card points with 2 tricks is better than 24 card point with 3 tricks.
/// ALWAYS CONTAINS POINTS FROM THE SKAT
pub struct YieldSoFar {
    card_points: CardPoints,
    number_of_tricks: u8,
}
impl YieldSoFar {
    // TODO: Make clear and assert that this has to contain the skat.
    pub fn new(card_points: CardPoints, number_of_tricks: u8) -> Self {
        Self {
            card_points,
            number_of_tricks,
        }
    }

    pub const MAX: Self = Self {
        card_points: CardPoints(120),
        number_of_tricks: 10,
    };

    pub const fn add_assign(&mut self, trick_yield: TrickYield) {
        self.card_points = CardPoints(self.card_points.0 + trick_yield.card_points.0);
        self.number_of_tricks += trick_yield.number_of_tricks
    }

    //TODO: Can that destroy the invariant that it is actually a yield from tricks?
    pub fn saturating_sub(self, other: Self) -> TrickYield {
        TrickYield {
            card_points: CardPoints(self.card_points.0.saturating_sub(other.card_points.0)),
            number_of_tricks: self.number_of_tricks.saturating_sub(other.number_of_tricks),
        }
    }

    pub fn card_points(self) -> CardPoints {
        self.card_points
    }

    pub fn number_of_tricks(self) -> u8 {
        self.number_of_tricks
    }

    pub const fn add(mut self, trick_yield: TrickYield) -> YieldSoFar {
        self.add_assign(trick_yield);
        self
    }
}

assert_eq_size!(TrickYield, u16);

impl TrickYield {
    // TODO: This is currently extremely unclear to me. Please write down the invariants.
    // TODO: Call it everywhere.
    const fn makes_probably_sense(self) -> bool {
        // if 121 <= self.card_points.0 {
        //     return self.number_of_tricks == 0;
        // }
        // if self.card_points.0 == 119 {
        //     return false;
        // }
        // if self.card_points.0 == 1 {
        //     return false;
        // }
        // if self.number_of_tricks == 10 {
        //     return 98 <= self.card_points.0;
        // }
        // if self.number_of_tricks == 0 {
        //     return self.card_points.0 <= 22;
        // }
        true
    }

    pub fn card_points(self) -> CardPoints {
        self.card_points
    }

    pub fn number_of_tricks(self) -> u8 {
        self.number_of_tricks
    }

    pub const ZERO_TRICKS: TrickYield = TrickYield {
        card_points: CardPoints(0),
        number_of_tricks: 0,
    };

    pub fn worst(minimax_role: MinimaxRole) -> Self {
        match minimax_role {
            MinimaxRole::Min => TrickYield {
                card_points: CardPoints(120),
                number_of_tricks: 10,
            },
            MinimaxRole::Max => Self::ZERO_TRICKS,
        }
    }

    pub fn from_trick(trick: Trick) -> Self {
        Self {
            card_points: trick.to_points(),
            number_of_tricks: 1,
        }
    }

    pub const fn saturating_sub(self, other: TrickYield) -> Self {
        Self {
            card_points: CardPoints(self.card_points.0.saturating_sub(other.card_points.0)),
            number_of_tricks: self.number_of_tricks.saturating_sub(other.number_of_tricks),
        }
    }

    pub const fn add_assign(&mut self, other: Self) {
        // TODO: CardPoints should get some of this functionality.
        self.card_points = CardPoints(self.card_points.0 + other.card_points.0);
        self.number_of_tricks += other.number_of_tricks;
        debug_assert!(self.makes_probably_sense());
    }

    // TODO: After invariants for `Self` are more clear, also have sensible constructors.
    // This is called from `quick_bounds` containing the skat.
    // Then, that gets substracted from MAX, so maybe there are two types, only one is trick yield?
    // Also, what about unrealistic thresholds, like reaching 34 points in one trick? Is this even expressible? Should it be?
    pub fn new(card_points: CardPoints, number_of_tricks: u8) -> Self {
        let result = Self {
            card_points,
            number_of_tricks,
        };
        debug_assert!(result.makes_probably_sense());
        result
    }

    pub fn sub(&self, delta: CardPoints) -> TrickYield {
        TrickYield {
            card_points: CardPoints(self.card_points.0.saturating_sub(delta.0)),
            number_of_tricks: self.number_of_tricks,
        }
    }

    pub fn add(&self, delta: CardPoints) -> TrickYield {
        TrickYield {
            card_points: CardPoints(self.card_points.0 + delta.0),
            number_of_tricks: self.number_of_tricks,
        }
    }
}
