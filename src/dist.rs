use derive_more::AddAssign;
use rand::{
    Rng,
    distr::{
        Distribution,
        uniform::{UniformSampler, UniformUsize},
    },
    seq::SliceRandom,
};

use crate::{
    bidding_role::BiddingRole,
    card::{Card, CardType, Suit},
    cards::Cards,
    deal::Deal,
    game_type::GameType,
    observed_gameplay::ObservedGameplay,
    situation::OpenSituation,
    util::choose,
};

#[derive(Clone, Copy)]
pub struct UnknownCards {
    number: u8,
    hearts_possible: bool,
    spades_possible: bool,
    diamonds_possible: bool,
    clubs_possible: bool,
    trump_possible: bool,
}

impl UnknownCards {
    pub const fn unrestricted(number: u8) -> Self {
        Self {
            number,
            hearts_possible: true,
            spades_possible: true,
            diamonds_possible: true,
            clubs_possible: true,
            trump_possible: true,
        }
    }

    pub const ALL_KNOWN: Self = Self {
        number: 0,
        hearts_possible: false,
        spades_possible: false,
        diamonds_possible: false,
        clubs_possible: false,
        trump_possible: false,
    };
    pub const UNKNOWN_HAND: Self = Self::unrestricted(10);
    pub const UNKNOWN_SKAT: Self = Self::unrestricted(2);

    fn possible(&self, card_type: CardType) -> bool {
        match card_type {
            CardType::Trump => self.trump_possible,
            CardType::Suit(Suit::Diamonds) => self.diamonds_possible,
            CardType::Suit(Suit::Spades) => self.spades_possible,
            CardType::Suit(Suit::Clubs) => self.clubs_possible,
            CardType::Suit(Suit::Hearts) => self.hearts_possible,
        }
    }

    fn max_possible(&self, card_type: CardType) -> u8 {
        if self.possible(card_type) {
            self.number
        } else {
            0
        }
    }

    fn without(mut self, number: u8) -> Self {
        self.remove(number);
        self
    }

    pub fn remove(&mut self, number: u8) {
        self.number = self.number.checked_sub(number).expect("valid operation");
    }

    fn is_empty(&self) -> bool {
        self.number == 0
    }

    pub fn retrict(&mut self, card_type: CardType) {
        match card_type {
            CardType::Trump => self.trump_possible = false,
            CardType::Suit(Suit::Clubs) => self.clubs_possible = false,
            CardType::Suit(Suit::Spades) => self.spades_possible = false,
            CardType::Suit(Suit::Hearts) => self.hearts_possible = false,
            CardType::Suit(Suit::Diamonds) => self.diamonds_possible = false,
        }
    }
}

#[derive(Clone, Copy, Debug, AddAssign)]
pub struct ColorDistribution {
    hearts: u8,
    spades: u8,
    diamonds: u8,
    clubs: u8,
    trump: u8,
}

impl ColorDistribution {
    pub const fn new(cards: Cards, game_type: GameType) -> Self {
        ColorDistribution {
            hearts: Cards::of_card_type(CardType::Suit(Suit::Hearts), game_type)
                .and(cards)
                .len(),
            spades: Cards::of_card_type(CardType::Suit(Suit::Spades), game_type)
                .and(cards)
                .len(),
            diamonds: Cards::of_card_type(CardType::Suit(Suit::Diamonds), game_type)
                .and(cards)
                .len(),
            clubs: Cards::of_card_type(CardType::Suit(Suit::Clubs), game_type)
                .and(cards)
                .len(),
            trump: Cards::of_card_type(CardType::Trump, game_type)
                .and(cards)
                .len(),
        }
    }

    pub const EMPTY: ColorDistribution = ColorDistribution {
        hearts: 0,
        spades: 0,
        diamonds: 0,
        clubs: 0,
        trump: 0,
    };

    pub fn initial(game_type: GameType) -> Self {
        match game_type {
            GameType::Trump(CardType::Suit(Suit::Clubs)) => ColorDistribution {
                hearts: 7,
                spades: 7,
                diamonds: 7,
                clubs: 0,
                trump: 11,
            },
            GameType::Trump(CardType::Suit(Suit::Hearts)) => ColorDistribution {
                hearts: 0,
                spades: 7,
                diamonds: 7,
                clubs: 7,
                trump: 11,
            },
            GameType::Trump(CardType::Suit(Suit::Diamonds)) => ColorDistribution {
                hearts: 7,
                spades: 7,
                diamonds: 0,
                clubs: 7,
                trump: 11,
            },
            GameType::Trump(CardType::Suit(Suit::Spades)) => ColorDistribution {
                hearts: 7,
                spades: 0,
                diamonds: 7,
                clubs: 7,
                trump: 11,
            },
            GameType::Trump(CardType::Trump) => ColorDistribution {
                hearts: 7,
                spades: 7,
                diamonds: 7,
                clubs: 7,
                trump: 4,
            },
            GameType::Null => ColorDistribution {
                hearts: 8,
                spades: 8,
                diamonds: 8,
                clubs: 8,
                trump: 0,
            },
        }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn len(&self) -> u8 {
        self.hearts + self.spades + self.diamonds + self.clubs + self.trump
    }

    fn get_mut(&mut self, card_type: CardType) -> &mut u8 {
        match card_type {
            CardType::Trump => &mut self.trump,
            CardType::Suit(Suit::Diamonds) => &mut self.diamonds,
            CardType::Suit(Suit::Spades) => &mut self.spades,
            CardType::Suit(Suit::Clubs) => &mut self.clubs,
            CardType::Suit(Suit::Hearts) => &mut self.hearts,
        }
    }

    fn without(mut self, number: u8, card_type: CardType) -> Self {
        let value = self.get_mut(card_type);
        *value = value.checked_sub(number).expect("valid operation");
        self
    }
}

struct PartialDistributionResult {
    distributed: ColorDistribution,
    still_open: ColorDistribution,
}

/// Assigns all unknown cards, returns still open cards.
fn partially_distribute(
    unknown_cards: &UnknownCards,
    open: &ColorDistribution,
) -> Vec<PartialDistributionResult> {
    let mut result = Vec::new();

    for h in 0..=unknown_cards
        .max_possible(CardType::Suit(Suit::Hearts))
        .min(open.hearts)
    {
        let unknown_cards = unknown_cards.without(h);
        let open = open.without(h, CardType::Suit(Suit::Hearts));

        for d in 0..=unknown_cards
            .max_possible(CardType::Suit(Suit::Diamonds))
            .min(open.diamonds)
        {
            let unknown_cards = unknown_cards.without(d);
            let open = open.without(d, CardType::Suit(Suit::Diamonds));

            for c in 0..=unknown_cards
                .max_possible(CardType::Suit(Suit::Clubs))
                .min(open.clubs)
            {
                let unknown_cards = unknown_cards.without(c);
                let open = open.without(c, CardType::Suit(Suit::Clubs));

                for s in 0..=unknown_cards
                    .max_possible(CardType::Suit(Suit::Spades))
                    .min(open.spades)
                {
                    let unknown_cards = unknown_cards.without(s);
                    let open = open.without(s, CardType::Suit(Suit::Spades));

                    for t in 0..=unknown_cards.max_possible(CardType::Trump).min(open.trump) {
                        let unknown_cards = unknown_cards.without(t);
                        let open = open.without(t, CardType::Trump);

                        if unknown_cards.is_empty() {
                            result.push(PartialDistributionResult {
                                distributed: ColorDistribution {
                                    hearts: h,
                                    spades: s,
                                    diamonds: d,
                                    clubs: c,
                                    trump: t,
                                },
                                still_open: open,
                            });
                        }
                    }
                }
            }
        }
    }

    result
}

/// Distributes all open cards to all unkonwn cards.
pub fn distribute_colors(
    unknown_cards_slice: &[UnknownCards],
    open: ColorDistribution,
) -> Vec<ColorDistributions> {
    let Some(unknown_cards) = unknown_cards_slice.last() else {
        return if open.is_empty() {
            vec![ColorDistributions::new()]
        } else {
            Vec::new()
        };
    };

    let mut result = Vec::new();

    for PartialDistributionResult {
        distributed,
        still_open,
    } in partially_distribute(unknown_cards, &open)
    {
        for mut possible_distribution in distribute_colors(
            &unknown_cards_slice[..unknown_cards_slice.len() - 1],
            still_open,
        ) {
            possible_distribution.push(distributed);
            result.push(possible_distribution);
        }
    }

    result
}

#[derive(Clone)]
struct OpenCards {
    hearts: Vec<Card>,
    spades: Vec<Card>,
    diamonds: Vec<Card>,
    clubs: Vec<Card>,
    trump: Vec<Card>,
}
impl OpenCards {
    pub fn ordered(open: Cards, game_type: GameType) -> Self {
        OpenCards {
            hearts: open
                .and(Cards::of_card_type(CardType::Suit(Suit::Hearts), game_type))
                .to_vec(),
            spades: open
                .and(Cards::of_card_type(CardType::Suit(Suit::Spades), game_type))
                .to_vec(),
            diamonds: open
                .and(Cards::of_card_type(
                    CardType::Suit(Suit::Diamonds),
                    game_type,
                ))
                .to_vec(),
            clubs: open
                .and(Cards::of_card_type(CardType::Suit(Suit::Clubs), game_type))
                .to_vec(),
            trump: open
                .and(Cards::of_card_type(CardType::Trump, game_type))
                .to_vec(),
        }
    }

    pub fn shuffle<R>(&mut self, rng: &mut R)
    where
        R: Rng + ?Sized,
    {
        self.clubs.shuffle(rng);
    }
}

fn choose_in_order(open_cards: &mut OpenCards, color_distributions: &ColorDistributions) -> Deal {
    let mut deal_iter = color_distributions.iter().map(|color_distribution| {
        let mut cards = Cards::EMPTY;

        for _ in 0..color_distribution.hearts {
            cards.add_new(open_cards.hearts.pop().expect("valid inputs"));
        }

        for _ in 0..color_distribution.diamonds {
            cards.add_new(open_cards.diamonds.pop().expect("valid inputs"));
        }

        for _ in 0..color_distribution.spades {
            cards.add_new(open_cards.spades.pop().expect("valid inputs"));
        }

        for _ in 0..color_distribution.clubs {
            cards.add_new(open_cards.clubs.pop().expect("valid inputs"));
        }

        for _ in 0..color_distribution.trump {
            cards.add_new(open_cards.trump.pop().expect("valid inputs"));
        }

        cards
    });

    let first_receiver = deal_iter.next().expect("valid data");
    let first_caller = deal_iter.next().expect("valid data");
    let second_caller = deal_iter.next().expect("valid data");
    let skat = deal_iter.next().expect("valid data");
    debug_assert!(deal_iter.next().is_none());

    Deal::new(first_receiver, first_caller, second_caller, skat)
}

pub struct UniformPossibleOpenSituationsFromObservedGameplay {
    possible_color_distributions_vec: Vec<ColorDistributions>,
    possibility_dist: UniformUsize,
    possible_situations: usize,
    open_cards_ordered: OpenCards,
    observed_cards: Deal,
    bidding_winner: BiddingRole,
}

impl UniformPossibleOpenSituationsFromObservedGameplay {
    pub fn color_distributions(&self) -> usize {
        self.possible_color_distributions_vec.len()
    }

    pub fn possible_situations(&self) -> usize {
        self.possible_situations
    }

    pub fn new(
        observed_gameplay: ObservedGameplay,
        // Required to be able to know what `Role` we are in the generated `OpenSituation`s
        bidding_winner: BiddingRole,
    ) -> Self {
        // TODO: Order important, should be better typed ...
        let (unknown_cards_vec, observed_cards) =
            observed_gameplay.unknown_cards_vec_and_observed_cards();

        let unobserved_cards = Cards::ALL.without(observed_cards.cards());
        let open = ColorDistribution::new(unobserved_cards, observed_gameplay.game_type());

        let possible_color_distributions_vec = distribute_colors(&unknown_cards_vec, open);

        let possible_situations = possible_color_distributions_vec
            .iter()
            .map(|color_distributions| color_distributions.possibilities())
            .sum();

        let possibility_dist = UniformUsize::new(0, possible_situations)
            .expect("there is a correct distribution, so possibilities is at least 1");

        let open_cards_ordered =
            OpenCards::ordered(unobserved_cards, observed_gameplay.game_type());

        Self {
            possible_color_distributions_vec,
            possibility_dist,
            open_cards_ordered,
            observed_cards,
            bidding_winner,
            possible_situations,
        }
    }
}

pub struct ColorDistributions {
    // If not exhausting, `possibilities` would not be correct.
    exhausting_color_distributions: Vec<ColorDistribution>,
}

impl Default for ColorDistributions {
    fn default() -> Self {
        Self::new()
    }
}

impl ColorDistributions {
    /// Distribute nothing via no `ColorDistribution`s.
    pub fn new() -> Self {
        Self {
            exhausting_color_distributions: Vec::new(),
        }
    }

    fn push(&mut self, distributed: ColorDistribution) {
        self.exhausting_color_distributions.push(distributed);
    }
}

impl ColorDistributions {
    pub fn possibilities(&self) -> usize {
        let mut distrution_so_far = ColorDistribution::EMPTY;
        let mut result = 1;

        for &color_distribution in &self.exhausting_color_distributions {
            distrution_so_far += color_distribution;
            result *= choose(distrution_so_far.clubs, color_distribution.clubs);
            result *= choose(distrution_so_far.hearts, color_distribution.hearts);
            result *= choose(distrution_so_far.diamonds, color_distribution.diamonds);
            result *= choose(distrution_so_far.spades, color_distribution.spades);
            result *= choose(distrution_so_far.trump, color_distribution.trump);
        }

        result
    }

    fn iter(&self) -> impl Iterator<Item = &ColorDistribution> {
        self.exhausting_color_distributions.iter()
    }
}

impl Distribution<OpenSituation> for UniformPossibleOpenSituationsFromObservedGameplay {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> OpenSituation {
        let mut distribution_id = self.possibility_dist.sample(rng);

        for color_distributions in &self.possible_color_distributions_vec {
            if color_distributions.possibilities() <= distribution_id {
                distribution_id -= color_distributions.possibilities();
                continue;
            }

            let mut open_cards = self.open_cards_ordered.clone();
            open_cards.shuffle(rng);

            let deal = choose_in_order(&mut open_cards, color_distributions);

            return OpenSituation::new(
                deal.combined_with_disjoint(self.observed_cards),
                self.bidding_winner,
            );
        }

        unreachable!("did not find any color distribution");
    }
}

#[cfg(test)]
mod tests {
    use crate::dist::{ColorDistribution, UnknownCards, distribute_colors};

    #[test]
    fn test_distribution_correct_numbers() {
        let unknown_cards_vec = vec![
            UnknownCards {
                number: 1,
                hearts_possible: true,
                spades_possible: false,
                diamonds_possible: true,
                clubs_possible: true,
                trump_possible: false,
            },
            UnknownCards {
                number: 2,
                hearts_possible: true,
                spades_possible: false,
                diamonds_possible: false,
                clubs_possible: false,
                trump_possible: false,
            },
            UnknownCards {
                number: 2,
                hearts_possible: true,
                spades_possible: true,
                diamonds_possible: true,
                clubs_possible: true,
                trump_possible: true,
            },
            UnknownCards {
                number: 3,
                hearts_possible: true,
                spades_possible: true,
                diamonds_possible: true,
                clubs_possible: false,
                trump_possible: false,
            },
        ];

        let right_amount = ColorDistribution {
            hearts: 3,
            spades: 2,
            diamonds: 1,
            clubs: 1,
            trump: 1,
        };

        let too_few = ColorDistribution {
            hearts: 2,
            spades: 2,
            diamonds: 1,
            clubs: 1,
            trump: 1,
        };

        let too_many = ColorDistribution {
            hearts: 4,
            spades: 2,
            diamonds: 1,
            clubs: 1,
            trump: 1,
        };

        assert!(distribute_colors(&unknown_cards_vec, too_few).is_empty());
        assert!(distribute_colors(&unknown_cards_vec, too_many).is_empty(),);
        assert_eq!(distribute_colors(&unknown_cards_vec, right_amount).len(), 5);
    }

    #[test]
    fn test_distribution_restrictions() {
        let one_no_hearts = UnknownCards {
            number: 1,
            hearts_possible: false,
            spades_possible: true,
            diamonds_possible: true,
            clubs_possible: true,
            trump_possible: true,
        };

        let open_heart = ColorDistribution {
            hearts: 1,
            spades: 0,
            diamonds: 0,
            clubs: 0,
            trump: 0,
        };

        let open_diamonds = ColorDistribution {
            hearts: 0,
            spades: 0,
            diamonds: 1,
            clubs: 0,
            trump: 0,
        };

        let open_trump = ColorDistribution {
            hearts: 0,
            spades: 0,
            diamonds: 0,
            clubs: 0,
            trump: 1,
        };

        assert!(distribute_colors(&[one_no_hearts], open_heart).is_empty());
        assert_eq!(distribute_colors(&[one_no_hearts], open_diamonds).len(), 1);
        assert_eq!(distribute_colors(&[one_no_hearts], open_trump).len(), 1);
    }

    #[test]
    fn test_initial_distributions() {
        let hand = UnknownCards {
            number: 10,
            hearts_possible: true,
            spades_possible: true,
            diamonds_possible: true,
            clubs_possible: true,
            trump_possible: true,
        };

        let skat = UnknownCards {
            number: 2,
            hearts_possible: true,
            spades_possible: true,
            diamonds_possible: true,
            clubs_possible: true,
            trump_possible: true,
        };

        let unknown_cards_vec = vec![hand, hand, hand, skat];

        let open_hearts = ColorDistribution {
            hearts: 0,
            spades: 7,
            diamonds: 7,
            clubs: 7,
            trump: 11,
        };

        let open_spades = ColorDistribution {
            hearts: 7,
            spades: 0,
            diamonds: 7,
            clubs: 7,
            trump: 11,
        };

        let open_diamonds = ColorDistribution {
            hearts: 7,
            spades: 7,
            diamonds: 0,
            clubs: 7,
            trump: 11,
        };

        let open_clubs = ColorDistribution {
            hearts: 7,
            spades: 7,
            diamonds: 7,
            clubs: 0,
            trump: 11,
        };

        let open_grand = ColorDistribution {
            hearts: 7,
            spades: 7,
            diamonds: 7,
            clubs: 7,
            trump: 4,
        };

        let open_null = ColorDistribution {
            hearts: 8,
            spades: 8,
            diamonds: 8,
            clubs: 8,
            trump: 0,
        };

        assert_eq!(
            distribute_colors(&unknown_cards_vec, open_clubs).len(),
            205_557
        );

        assert_eq!(
            distribute_colors(&unknown_cards_vec, open_diamonds).len(),
            205_557
        );

        assert_eq!(
            distribute_colors(&unknown_cards_vec, open_spades).len(),
            205_557
        );

        assert_eq!(
            distribute_colors(&unknown_cards_vec, open_hearts).len(),
            205_557
        );

        assert_eq!(
            distribute_colors(&unknown_cards_vec, open_grand).len(),
            2_337_894
        );

        assert_eq!(
            distribute_colors(&unknown_cards_vec, open_null).len(),
            248_358
        );
    }

    #[test]
    fn test_distribution_sl003() {
        let hand = UnknownCards {
            number: 10,
            hearts_possible: true,
            spades_possible: true,
            diamonds_possible: true,
            clubs_possible: true,
            trump_possible: true,
        };

        let unknown_cards_vec = vec![hand, hand];

        let open = ColorDistribution {
            hearts: 5,
            spades: 6,
            diamonds: 4,
            clubs: 0,
            trump: 5,
        };

        assert_eq!(distribute_colors(&unknown_cards_vec, open).len(), 140);
    }
}
