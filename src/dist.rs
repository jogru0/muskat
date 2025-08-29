use crate::card::{CardType, Suit};

#[derive(Clone, Copy)]
pub struct UnknownCards {
    number: usize,
    hearts_possible: bool,
    spades_possible: bool,
    diamonds_possible: bool,
    clubs_possible: bool,
    trump_possible: bool,
}

impl UnknownCards {
    fn possible(&self, card_type: CardType) -> bool {
        match card_type {
            CardType::Trump => self.trump_possible,
            CardType::Suit(Suit::Diamonds) => self.diamonds_possible,
            CardType::Suit(Suit::Spades) => self.spades_possible,
            CardType::Suit(Suit::Clubs) => self.clubs_possible,
            CardType::Suit(Suit::Hearts) => self.hearts_possible,
        }
    }

    fn max_possible(&self, card_type: CardType) -> usize {
        if self.possible(card_type) {
            self.number
        } else {
            0
        }
    }

    fn without(mut self, number: usize) -> Self {
        self.number = self.number.checked_sub(number).expect("valid operation");
        self
    }

    fn is_empty(&self) -> bool {
        self.number == 0
    }
}

#[derive(Clone, Copy, Debug)]
pub struct ColorDistribution {
    hearts: usize,
    spades: usize,
    diamonds: usize,
    clubs: usize,
    trump: usize,
}
impl ColorDistribution {
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn len(&self) -> usize {
        self.hearts + self.spades + self.diamonds + self.clubs + self.trump
    }

    fn get_mut(&mut self, card_type: CardType) -> &mut usize {
        match card_type {
            CardType::Trump => &mut self.trump,
            CardType::Suit(Suit::Diamonds) => &mut self.diamonds,
            CardType::Suit(Suit::Spades) => &mut self.spades,
            CardType::Suit(Suit::Clubs) => &mut self.clubs,
            CardType::Suit(Suit::Hearts) => &mut self.hearts,
        }
    }

    fn without(mut self, number: usize, card_type: CardType) -> Self {
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
) -> Vec<Vec<ColorDistribution>> {
    let Some(unknown_cards) = unknown_cards_slice.last() else {
        return if open.is_empty() {
            vec![Vec::new()]
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
