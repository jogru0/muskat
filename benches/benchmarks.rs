use std::hint::black_box;

use criterion::{BatchSize, Criterion, criterion_group, criterion_main};
use muskat::{
    card::{CardType, Suit},
    cards::Cards,
    game_type::GameType,
    open_analysis_performance::{
        OpenSituationAndGameType, analyzer_conclusion, generate_random_unfinished_open_situations,
    },
    open_situation_solver::{
        OpenSituationSolver,
        bounds_cache::{FastOpenSituationSolverCache, open_situation_reachable_from_to_u32_key},
    },
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

    c.bench_function("random still_makes_at_least", |b| {
        let mut open_situation_iter = generate_random_unfinished_open_situations(&mut rng);
        b.iter_batched(
            || {
                let OpenSituationAndGameType {
                    open_situation,
                    game_type,
                    yield_so_far,
                } = open_situation_iter.next().expect("infinite iterator");
                let solver = OpenSituationSolver::new(
                    FastOpenSituationSolverCache::new(
                        open_situation_reachable_from_to_u32_key(open_situation),
                        game_type,
                    ),
                    game_type,
                );
                (solver, open_situation, yield_so_far)
            },
            |(mut solver, open_situation, yield_so_far)| {
                analyzer_conclusion(open_situation, &mut solver, yield_so_far)
            },
            BatchSize::SmallInput,
        )
    });

    c.bench_function("quick_bounds forehand", |b| {
        let mut open_situation_iter = generate_random_unfinished_open_situations(&mut rng)
            .filter_map(
                |OpenSituationAndGameType {
                     open_situation,
                     game_type,
                     yield_so_far: _,
                 }| {
                    if open_situation.is_trick_in_progress() {
                        None
                    } else {
                        Some((open_situation, game_type))
                    }
                },
            );
        b.iter_batched(
            || open_situation_iter.next().expect("infinite iterator"),
            |(open_situation, game_type)| {
                open_situation.quick_bounds_trick_not_in_progress(game_type)
            },
            BatchSize::SmallInput,
        )
    });

    c.bench_function("quick_bounds not forehand", |b| {
        let mut open_situation_iter = generate_random_unfinished_open_situations(&mut rng)
            .filter_map(
                |OpenSituationAndGameType {
                     open_situation,
                     game_type: _,
                     yield_so_far: _,
                 }| {
                    if open_situation.is_trick_in_progress() {
                        Some(open_situation)
                    } else {
                        None
                    }
                },
            );
        b.iter_batched(
            || open_situation_iter.next().expect("infinite iterator"),
            |open_situation| open_situation.quick_bounds_trick_in_progress(),
            BatchSize::SmallInput,
        )
    });
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
