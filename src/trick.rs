use crate::{
    card::Card, card_points::CardPoints, game_type::GameType, position::Position, power::CardPower,
};
use static_assertions::assert_eq_size;

#[derive(Clone, Copy)]
pub struct Trick {
    first: Card,
    second: Card,
    third: Card,
}

assert_eq_size!(Trick, [u8; 3]);

impl Trick {
    pub fn new(first: Card, second: Card, third: Card) -> Self {
        debug_assert_ne!(first, second);
        debug_assert_ne!(first, third);
        debug_assert_ne!(second, third);

        Self {
            first,
            second,
            third,
        }
    }

    pub fn winner_position(self, game_type: GameType) -> Position {
        let power_forehand = CardPower::of(self.first, self.first, game_type);
        let power_middlehand = CardPower::of(self.second, self.first, game_type);
        let power_rearhand = CardPower::of(self.third, self.first, game_type);

        debug_assert!(!power_forehand.comes_from_deactivated_card());
        debug_assert_ne!(power_forehand, power_middlehand);
        debug_assert_ne!(power_forehand, power_rearhand);
        debug_assert!(
            power_middlehand != power_rearhand || power_middlehand.comes_from_deactivated_card()
        );

        // TODO: Where is the lint for return?
        if power_forehand <= power_middlehand {
            if power_middlehand <= power_rearhand {
                Position::Rearhand
            } else {
                Position::Middlehand
            }
        } else if power_forehand <= power_rearhand {
            Position::Rearhand
        } else {
            Position::Forehand
        }
    }

    pub const fn to_points(self) -> CardPoints {
        CardPoints(self.first.to_points().0 + self.second.to_points().0 + self.third.to_points().0)
    }
}
