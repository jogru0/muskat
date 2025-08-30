use static_assertions::assert_eq_size;

use crate::bidding_role::BiddingRole;

#[derive(Clone, Copy)]
pub enum Position {
    Forehand,
    Middlehand,
    Rearhand,
}

assert_eq_size!(Position, u8);

impl Position {
    pub fn role(self, first_player: BiddingRole) -> BiddingRole {
        match self {
            Position::Forehand => first_player,
            Position::Middlehand => first_player.next(),
            Position::Rearhand => first_player.next().next(),
        }
    }
}
