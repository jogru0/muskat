use static_assertions::assert_eq_size;

use crate::trick_yield::TrickYield;

/// With perfect play, the TrickYield is at least lower, at most upper.
#[derive(Clone, Copy, Debug)]
pub struct Bounds {
    lower: TrickYield,
    upper: TrickYield,
}

assert_eq_size!(Bounds, u32);

impl Bounds {
    pub fn new(lower: TrickYield, upper: TrickYield) -> Self {
        debug_assert!(lower <= upper);
        Self { lower, upper }
    }

    pub fn decides_threshold(self, threshold: TrickYield) -> bool {
        threshold <= self.lower || self.upper < threshold
    }

    pub fn lower(self) -> TrickYield {
        self.lower
    }

    pub fn upper(self) -> TrickYield {
        self.upper
    }

    pub fn minimize_upper(&mut self, upper_bound: TrickYield) {
        self.upper = self.upper.min(upper_bound);
        debug_assert!(self.lower <= self.upper);
    }

    pub fn maximize_lower(&mut self, lower_bound: TrickYield) {
        self.lower = self.lower.max(lower_bound);
        debug_assert!(self.lower <= self.upper);
    }

    pub fn update_lower(&mut self, lower_bound: TrickYield) {
        debug_assert!(self.lower < lower_bound);
        self.lower = lower_bound;
        debug_assert!(self.lower <= self.upper)
    }

    pub fn update_upper(&mut self, upper_bound: TrickYield) {
        debug_assert!(upper_bound < self.upper);
        self.upper = upper_bound;
        debug_assert!(self.lower <= self.upper)
    }
}
