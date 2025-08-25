use static_assertions::assert_eq_size;

use crate::{bounds::Bounds, card::Card};

#[derive(Clone, Copy)]
pub struct BoundsAndMaybePreference {
    bounds: Bounds,
    maybe_preference: Option<Card>,
}

assert_eq_size!(BoundsAndMaybePreference, [u8; 5]);

impl BoundsAndMaybePreference {
    pub fn bounds(self) -> Bounds {
        self.bounds
    }

    pub fn maybe_preference(self) -> Option<Card> {
        self.maybe_preference
    }

    pub(crate) fn new(bounds: Bounds, maybe_preference: Option<Card>) -> Self {
        Self {
            bounds,
            maybe_preference,
        }
    }
}
