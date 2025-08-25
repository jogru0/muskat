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

    pub const NONE: TrickYield = TrickYield {
        card_points: CardPoints(0),
        number_of_tricks: 0,
    };

    pub const MAX: TrickYield = TrickYield {
        card_points: CardPoints(120),
        number_of_tricks: 10,
    };

    pub fn worst(minimax_role: MinimaxRole) -> Self {
        match minimax_role {
            MinimaxRole::Min => TrickYield::MAX,
            MinimaxRole::Max => TrickYield::NONE,
        }
    }

    pub const fn from_trick(trick: Trick) -> TrickYield {
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

    pub fn card_points(self) -> CardPoints {
        self.card_points
    }

    pub fn number_of_tricks(self) -> u8 {
        self.number_of_tricks
    }
}
