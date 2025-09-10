use std::hint::black_box;

use criterion::{BatchSize, Criterion, criterion_group, criterion_main};
use muskat::{
    card::{CardType, Suit},
    cards::Cards,
    game_type::GameType,
    rng::cheap_rng,
    trick::Trick,
};

fn criterion_benchmark(c: &mut Criterion) {
    let mut rng = cheap_rng(3443289672);

    c.bench_function("Cards::to_points random", |b| {
        b.iter_batched(
            || Cards::random(&mut rng),
            |cards| cards.to_points(),
            BatchSize::SmallInput,
        )
    });

    c.bench_function("Trick::to_points random", |b| {
        b.iter_batched(
            || Trick::random(&mut rng),
            |trick| trick.to_points(),
            BatchSize::SmallInput,
        )
    });

    c.bench_function("winner_position diamonds random", |b| {
        let game_type = GameType::Trump(CardType::Suit(Suit::Diamonds));

        b.iter_batched(
            || Trick::random(&mut rng),
            |trick| trick.winner_position(game_type),
            BatchSize::SmallInput,
        )
    });

    c.bench_function("iterate Cards to end", |b| {
        b.iter_batched(
            || Cards::random(&mut rng),
            |cards| {
                for card in cards {
                    black_box(card);
                }
            },
            BatchSize::SmallInput,
        )
    });

    c.bench_function("Cards::of_type random", |b| {
        b.iter_batched(
            || (CardType::random(&mut rng), GameType::random(&mut rng)),
            |(card_type, game_type)| Cards::of_card_type(card_type, game_type),
            BatchSize::SmallInput,
        )
    });
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
