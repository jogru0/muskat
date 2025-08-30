use crate::bidding_role::BiddingRole;
use static_assertions::assert_eq_size;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Role {
    Declarer,
    FirstDefender,
    SecondDefender,
}

assert_eq_size!(Role, u8);

impl Role {
    pub fn next(self) -> Self {
        match self {
            Role::Declarer => Role::FirstDefender,
            Role::FirstDefender => Role::SecondDefender,
            Role::SecondDefender => Role::Declarer,
        }
    }

    pub fn first_active(bidding_winner: BiddingRole) -> Self {
        match bidding_winner {
            BiddingRole::FirstReceiver => Self::Declarer,
            BiddingRole::FirstCaller => Self::SecondDefender,
            BiddingRole::SecondCaller => Self::FirstDefender,
        }
    }
}
