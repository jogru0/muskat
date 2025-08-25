use static_assertions::assert_eq_size;

use crate::{game_type::GameType, role::Role};

#[derive(Clone, Copy)]
pub enum MinimaxRole {
    Min,
    Max,
}

assert_eq_size!(MinimaxRole, u8);

impl MinimaxRole {
    pub fn of(role: Role, game_type: GameType) -> Self {
        let is_declarer = matches!(role, Role::Declarer);
        let is_null = matches!(game_type, GameType::Null);

        match is_declarer != is_null {
            true => Self::Max,
            false => Self::Min,
        }
    }
}
