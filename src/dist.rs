use derive_more::AddAssign;
use itertools::Itertools;
use rand::{
    Rng,
    distr::{
        Distribution,
        uniform::{UniformSampler, UniformUsize},
    },
    seq::SliceRandom,
};

use crate::{
    card::{Card, CardType, Suit},
    cards::Cards,
    deal::Deal,
    game_type::GameType,
    observed_gameplay::CardKnowledge,
    util::{all_choices, choose},
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

    pub fn is_empty(&self) -> bool {
        self.number == 0
    }

    pub fn retrict(&mut self, trick_type: CardType) {
        match trick_type {
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

fn remove_by_index(to_be_shortened: &mut Vec<Card>, sorted_indices_to_remove: Vec<usize>) -> Cards {
    let mut i = 0;
    let mut j = 0;
    let mut removed = Cards::EMPTY;

    to_be_shortened.retain(|&card| {
        let old_i = i;
        i += 1;

        if let Some(&next_removal_i) = sorted_indices_to_remove.get(j)
            && old_i == next_removal_i
        {
            removed.add_new(card);
            j += 1;
            return false;
        }

        true
    });

    assert_eq!(i, to_be_shortened.len() + removed.len() as usize);
    assert_eq!(j, removed.len() as usize);

    removed
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

    fn get_all(&self, open_cards: &OpenCards) -> Vec<(Cards, OpenCards)> {
        let mut result = Vec::new();

        for hearts_choices in all_choices(open_cards.hearts.len() as u8, self.hearts) {
            let mut remaining_hearts = open_cards.hearts.clone();
            let selected_hearts = remove_by_index(&mut remaining_hearts, hearts_choices);

            for diamonds_choices in all_choices(open_cards.diamonds.len() as u8, self.diamonds) {
                let mut remaining_diamonds = open_cards.diamonds.clone();
                let selected_diamonds = remove_by_index(&mut remaining_diamonds, diamonds_choices);

                for spades_choices in all_choices(open_cards.spades.len() as u8, self.spades) {
                    let mut remaining_spades = open_cards.spades.clone();
                    let selected_spades = remove_by_index(&mut remaining_spades, spades_choices);

                    for clubs_choices in all_choices(open_cards.clubs.len() as u8, self.clubs) {
                        let mut remaining_clubs = open_cards.clubs.clone();
                        let selected_clubs = remove_by_index(&mut remaining_clubs, clubs_choices);

                        for trump_choices in all_choices(open_cards.trump.len() as u8, self.trump) {
                            let mut remaining_trump = open_cards.trump.clone();
                            let selected_trump =
                                remove_by_index(&mut remaining_trump, trump_choices);

                            result.push((
                                selected_hearts
                                    .combined_with_disjoint(selected_clubs)
                                    .combined_with_disjoint(selected_diamonds)
                                    .combined_with_disjoint(selected_spades)
                                    .combined_with_disjoint(selected_trump),
                                OpenCards {
                                    hearts: remaining_hearts.clone(),
                                    spades: remaining_spades.clone(),
                                    diamonds: remaining_diamonds.clone(),
                                    clubs: remaining_clubs.clone(),
                                    trump: remaining_trump,
                                },
                            ));
                        }
                    }
                }
            }
        }

        result
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
    pub fn is_empty(&self) -> bool {
        self.clubs.is_empty()
            && self.spades.is_empty()
            && self.hearts.is_empty()
            && self.trump.is_empty()
            && self.diamonds.is_empty()
    }

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
        self.hearts.shuffle(rng);
        self.diamonds.shuffle(rng);
        self.spades.shuffle(rng);
        self.trump.shuffle(rng);
    }
}

fn choose_in_order(
    open_cards: &mut OpenCards,
    color_distributions: &ColorDistributions,
) -> [Cards; 4] {
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

    [first_receiver, first_caller, second_caller, skat]
}

#[derive(Clone)]
pub struct UniformPossibleDealsFromObservedGameplay {
    possible_color_distributions_vec: Vec<ColorDistributions>,
    possibility_dist: UniformUsize,
    number_of_possibilities: usize,
    open_cards_ordered: OpenCards,
    observed_cards_array: [Cards; 4],
}

impl UniformPossibleDealsFromObservedGameplay {
    pub fn get_all_possibilities(&self) -> Vec<Deal> {
        let mut result = Vec::new();

        for possible_color_distributions in &self.possible_color_distributions_vec {
            result.extend(
                possible_color_distributions
                    .get_all_possibilities(&self.open_cards_ordered)
                    .into_iter()
                    .map(|vec| {
                        Deal::new(
                            vec[0].combined_with_disjoint(self.observed_cards_array[0]),
                            vec[1].combined_with_disjoint(self.observed_cards_array[1]),
                            vec[2].combined_with_disjoint(self.observed_cards_array[2]),
                            vec[3].combined_with_disjoint(self.observed_cards_array[3]),
                        )
                    }),
            );
        }

        debug_assert_eq!(result.len(), self.number_of_possibilities());
        debug_assert!(result.iter().all_unique());

        result
    }

    fn sample_color_distribution<R>(&self, rng: &mut R) -> &ColorDistributions
    where
        R: Rng + ?Sized,
    {
        let mut id = self.possibility_dist.sample(rng);

        for color_distributions in &self.possible_color_distributions_vec {
            match id.checked_sub(color_distributions.number_of_possibilities()) {
                Some(new_id) => id = new_id,
                None => return color_distributions,
            }
        }

        unreachable!("out of bounds for possible color distributions")
    }

    pub fn number_of_color_distributions(&self) -> usize {
        self.possible_color_distributions_vec.len()
    }

    pub fn number_of_possibilities(&self) -> usize {
        self.number_of_possibilities
    }

    pub fn new(card_knowledge: &CardKnowledge, game_type: GameType) -> Self {
        let unobserved_cards = Cards::ALL.without(card_knowledge.observed_cards());
        let open: ColorDistribution = ColorDistribution::new(unobserved_cards, game_type);

        let possible_color_distributions_vec =
            distribute_colors(card_knowledge.unknown_cards_slice(), open);

        let possible_situations = possible_color_distributions_vec
            .iter()
            .map(|color_distributions| color_distributions.number_of_possibilities())
            .sum();

        let possibility_dist = UniformUsize::new(0, possible_situations)
            .expect("there is a correct distribution, so possibilities is at least 1");

        let open_cards_ordered = OpenCards::ordered(unobserved_cards, game_type);

        let observed_cards_array = card_knowledge.observed_cards_array();

        Self {
            possible_color_distributions_vec,
            possibility_dist,
            open_cards_ordered,
            observed_cards_array,
            number_of_possibilities: possible_situations,
        }
    }
}

#[derive(Clone)]
pub struct ColorDistributions {
    // If not exhausting, `possibilities` would not be correct.
    exhausting_color_distributions: Vec<ColorDistribution>,
}

impl Default for ColorDistributions {
    fn default() -> Self {
        Self::new()
    }
}

//TODO: Return iterator
fn get_all_possibilities_impl(
    exhausting_color_distributions: &[ColorDistribution],
    open_cards: &OpenCards,
) -> Vec<Vec<Cards>> {
    let Some(last_color_distribution) = exhausting_color_distributions.last() else {
        debug_assert!(open_cards.is_empty());
        return vec![Vec::new()];
    };

    let mut result = Vec::new();

    for (distributed_cards, remaining_open_cards) in last_color_distribution.get_all(open_cards) {
        for mut remaining_distributions in get_all_possibilities_impl(
            &exhausting_color_distributions[..exhausting_color_distributions.len() - 1],
            &remaining_open_cards,
        ) {
            remaining_distributions.push(distributed_cards);
            result.push(remaining_distributions);
        }
    }

    result
}

impl ColorDistributions {
    //TODO: Return iterator
    fn get_all_possibilities(&self, open_cards: &OpenCards) -> Vec<Vec<Cards>> {
        let result = get_all_possibilities_impl(&self.exhausting_color_distributions, open_cards);
        assert_eq!(result.len(), self.number_of_possibilities());
        assert!(result.iter().all_unique());
        result
    }

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
    pub fn number_of_possibilities(&self) -> usize {
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

impl Distribution<Deal> for UniformPossibleDealsFromObservedGameplay {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> Deal {
        let color_distributions: &ColorDistributions = self.sample_color_distribution(rng);

        let mut open_cards = self.open_cards_ordered.clone();
        open_cards.shuffle(rng);

        let chosen = choose_in_order(&mut open_cards, color_distributions);

        Deal::new(
            chosen[0].combined_with_disjoint(self.observed_cards_array[0]),
            chosen[1].combined_with_disjoint(self.observed_cards_array[1]),
            chosen[2].combined_with_disjoint(self.observed_cards_array[2]),
            chosen[3].combined_with_disjoint(self.observed_cards_array[3]),
        )
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        card::Card,
        dist::{ColorDistribution, OpenCards, UnknownCards, distribute_colors},
    };

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

        let color_distributions_vec = distribute_colors(&unknown_cards_vec, right_amount);

        assert_eq!(color_distributions_vec.len(), 5);

        let open_cards = OpenCards {
            hearts: vec![Card::H7, Card::H8, Card::H9],
            spades: vec![Card::G7, Card::GZ],
            diamonds: vec![Card::SK],
            clubs: vec![Card::EA],
            trump: vec![Card::EU],
        };

        for color_distributions in color_distributions_vec {
            assert_eq!(
                color_distributions.get_all_possibilities(&open_cards).len(),
                color_distributions.number_of_possibilities()
            );
        }
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
