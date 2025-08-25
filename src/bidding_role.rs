use static_assertions::assert_eq_size;
use strum::VariantArray;

#[derive(Clone, Copy, VariantArray)]
pub enum BiddingRole {
    FirstReceiver,
    FirstCaller,
    SecondCaller,
}

assert_eq_size!(BiddingRole, u8);

impl BiddingRole {
    pub const DEALER: Self = Self::SecondCaller;
    // TODO: There are tests waiting for this, verifying that the right person starts in a new situation, I think.
    pub const FIRST_ACTIVE_PLAYER: Self = Self::FirstReceiver;

    pub const SECOND_RECEIVER: Self = Self::FirstCaller;

    pub fn next(self) -> Self {
        match self {
            Self::FirstReceiver => Self::FirstCaller,
            Self::FirstCaller => Self::SecondCaller,
            Self::SecondCaller => Self::FirstReceiver,
        }
    }
}
