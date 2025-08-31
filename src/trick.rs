use crate::{
    card::{Card, CardType},
    card_points::CardPoints,
    cards::Cards,
    game_type::GameType,
    position::Position,
    power::CardPower,
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

    pub fn first(self) -> Card {
        self.first
    }

    pub fn second(self) -> Card {
        self.second
    }

    pub fn third(self) -> Card {
        self.third
    }

    pub fn trick_type(self, game_type: GameType) -> CardType {
        self.first.card_type(game_type)
    }

    pub fn winner_position(self, game_type: GameType) -> Position {
        let trick_type = self.trick_type(game_type);

        let power_forehand = CardPower::of(self.first, trick_type, game_type);
        let power_middlehand = CardPower::of(self.second, trick_type, game_type);
        let power_rearhand = CardPower::of(self.third, trick_type, game_type);

        debug_assert!(!power_forehand.comes_from_deactivated_card());
        debug_assert_ne!(power_forehand, power_middlehand);
        debug_assert_ne!(power_forehand, power_rearhand);
        debug_assert!(
            power_middlehand != power_rearhand || power_middlehand.comes_from_deactivated_card()
        );

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

    pub fn to_points(self) -> CardPoints {
        self.cards().to_points()
    }

    pub const fn cards(&self) -> Cards {
        let mut result = Cards::EMPTY;
        result.add_new(self.first);
        result.add_new(self.second);
        result.add_new(self.third);
        result
    }
}
