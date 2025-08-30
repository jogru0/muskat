use rand::{Rng, SeedableRng as _};
use rand_xoshiro::Xoshiro256PlusPlus;

pub fn cheap_rng(seed: u64) -> impl Rng {
    Xoshiro256PlusPlus::seed_from_u64(seed)
}
