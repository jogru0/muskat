use static_assertions::assert_eq_size;

use crate::{card_points::CardPoints, trick_yield::TrickYield};

/// With perfect play, the TrickYield is at least lower, at most upper.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
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

    // TODO: Write test making sure this is strict, in particular if number_of_tricks make the obvious bounds not tight.
    // After these are stricter, enable the assertions again that no quasi symmetric cards are banned even though they would
    // Be on the sought after side of the threshold.
    /// Meaning the bounds could be the distance larger in both directions and still decide the threshold.
    ///
    /// The most amount of card points you can widen the bounds (without changing the value for number_of_tricks)
    /// to still decide the threshold
    pub fn distance_to_threshold(self, threshold: TrickYield) -> CardPoints {
        debug_assert!(self.decides_threshold(threshold));

        // TODO: Have to do saturating sub because of stuff with number of tricks ...
        // We really should make sure we don't have sublte bugs related to stuff like this!
        let th = threshold.card_points().0;
        let result = CardPoints(
            th.checked_sub(self.upper.card_points().0 + 1)
                .unwrap_or_else(|| self.lower.card_points().0.saturating_sub(th)),
        );

        debug_assert!(
            Bounds::new(self.lower.sub(result), self.upper.add(result))
                .decides_threshold(threshold),
            "self: {:?}, threshold: {:?}, result: {:?}, new_bounds: {:?}",
            self,
            threshold,
            result,
            Bounds::new(self.lower.sub(result), self.upper.add(result))
        );
        result
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
