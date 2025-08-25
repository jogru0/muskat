use static_assertions::assert_eq_size;

pub enum Position {
    Forehand,
    Middlehand,
    Rearhand,
}

assert_eq_size!(Position, u8);
