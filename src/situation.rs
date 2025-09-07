use static_assertions::assert_eq_size;

use crate::{
    bidding_role::BiddingRole,
    bounds::Bounds,
    card::{Card, CardType, Rank, Suit},
    card_points::CardPoints,
    cards::Cards,
    deal::Deal,
    game_type::GameType,
    minimax_role::MinimaxRole,
    partial_trick::PartialTrick,
    position::Position,
    role::Role,
    trick_yield::{TrickYield, YieldSoFar},
};

/// This is meant to be used to extensively analyze possible plays in OpenSituationSolver
/// regarding expected future trick yield, so it needs to be optimized for that:
/// * does not contain history like bidding information or trick yields so far, or even the game type
/// * fast to deduce possible next moves, get resulting child situations, etc.
// TODO: Check trading of memory footprint/speed of playing a card vs having to recalculate certain stuff when needed
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct OpenSituation {
    hand_declarer: Cards,
    hand_first_defender: Cards,
    hand_second_defender: Cards,
    partial_trick: PartialTrick,
    active_role: Role,
}

assert_eq_size!(OpenSituation, u128);

impl OpenSituation {
    pub fn is_trick_in_progress(self) -> bool {
        self.partial_trick.is_in_progress()
    }

    pub fn is_initial_situation(self) -> bool {
        self.cellar().len() == 2 && matches!(self.partial_trick, PartialTrick::EMPTY)
    }

    pub fn maybe_first_trick_card(self) -> Option<Card> {
        self.partial_trick.first()
    }

    pub fn active_role(self) -> Role {
        self.active_role
    }

    pub fn active_minimax_role(self, game_type: GameType) -> MinimaxRole {
        MinimaxRole::of(self.active_role(), game_type)
    }

    /// Assumed to not be used on empty hand.
    ///
    /// Therefore guaranteed to return at least one option.
    pub fn next_possible_plays(self, game_type: GameType) -> Cards {
        let hand_cards = self.active_hand_cards();

        hand_cards.possible_plays(self.partial_trick.first(), game_type)
    }

    pub const fn hand_cards_of(self, role: Role) -> Cards {
        match role {
            Role::Declarer => self.hand_declarer,
            Role::FirstDefender => self.hand_first_defender,
            Role::SecondDefender => self.hand_second_defender,
        }
    }

    fn active_hand_cards_mut(&mut self) -> &mut Cards {
        match self.active_role {
            Role::Declarer => &mut self.hand_declarer,
            Role::FirstDefender => &mut self.hand_first_defender,
            Role::SecondDefender => &mut self.hand_second_defender,
        }
    }

    fn active_hand_cards(self) -> Cards {
        self.hand_cards_of(self.active_role)
    }

    /// Returns what the declarer yields with this move.
    #[must_use]
    pub fn play_card(&mut self, card: Card, game_type: GameType) -> TrickYield {
        debug_assert!(self.next_possible_plays(game_type).contains(card));
        let hand = self.active_hand_cards_mut();
        hand.remove_existing(card);

        self.active_role = self.active_role.next();

        let Some(trick) = self.partial_trick.add(card) else {
            self.assert_invariants();
            return TrickYield::ZERO_TRICKS;
        };

        let position = trick.winner_position(game_type);

        //Active role currently is same as `Position::Forehand`.
        match position {
            Position::Forehand => {}
            Position::Middlehand => self.active_role = self.active_role.next(),
            Position::Rearhand => self.active_role = self.active_role.next().next(),
        }

        self.assert_invariants();

        if matches!(self.active_role, Role::Declarer) {
            TrickYield::from_trick(trick)
        } else {
            TrickYield::ZERO_TRICKS
        }
    }

    fn assert_invariants(self) {
        let number_of_appearing_cards = self.hand_declarer.len()
            + self.hand_first_defender.len()
            + self.hand_second_defender.len()
            + self.partial_trick.number_of_cards();

        //All cards appear at most once.
        debug_assert_eq!(number_of_appearing_cards + self.cellar().len(), 32);

        let number_cards_belonging_to_p = self.hand_cards_of(self.active_role).len();
        let number_cards_belonging_to_np = self.hand_cards_of(self.active_role.next()).len()
            + self.partial_trick.second().map(|_| 1).unwrap_or(0);
        let number_cards_belonging_to_nnp =
            self.hand_cards_of(self.active_role.next().next()).len()
                + self.partial_trick.first().map(|_| 1).unwrap_or(0);

        debug_assert_eq!(number_cards_belonging_to_p, number_cards_belonging_to_np);
        debug_assert_eq!(number_cards_belonging_to_p, number_cards_belonging_to_nnp);
    }

    // TODO: For upper bound, can remove points from opposing matadors.

    pub fn lower_bound_non_null_declarer_forehand(mut self, trump: CardType) -> TrickYield {
        debug_assert_eq!(self.active_role, Role::Declarer);
        debug_assert!(matches!(self.partial_trick, PartialTrick::EMPTY));

        let game_type = GameType::Trump(trump);
        let trump_cards = Cards::of_card_type(CardType::Trump, game_type);

        let mut count_f = 0;
        let mut count_s = 0;

        let mut points = CardPoints(0);
        let mut tricks = 0;

        let remaining_cards = self
            .hand_declarer
            .combined_with_disjoint(self.hand_first_defender)
            .combined_with_disjoint(self.hand_second_defender);

        let mut remaining_trump = remaining_cards.and(trump_cards);
        let mut count_f_trump = 0;
        let mut hand_f = self.hand_first_defender;
        let mut count_s_trump = 0;
        let mut hand_s = self.hand_second_defender;

        while let Some(matador) = remaining_trump.remove_highest_non_null()
            && self.hand_declarer.remove_if_there(matador)
        {
            tricks += 1;
            points += matador.to_points();

            if let Some(low_trump) = hand_f.remove_lowest_of_type(CardType::Trump, game_type) {
                remaining_trump.remove_existing(low_trump);
                count_f_trump += 1;
            } else {
                count_f += 1;
            }

            if let Some(low_trump) = hand_s.remove_lowest_of_type(CardType::Trump, game_type) {
                remaining_trump.remove_existing(low_trump);
                count_s_trump += 1;
            } else {
                count_s += 1;
            }
        }

        for _ in 0..count_f_trump {
            let mut available = self.hand_first_defender.and(Cards::of_trump(game_type));
            let card = available
                .and(Cards::OF_ZERO_POINTS)
                .remove_smallest()
                .or_else(|| {
                    available
                        .and(Cards::of_rank(crate::card::Rank::U))
                        .remove_smallest()
                })
                .unwrap_or_else(|| unsafe { available.remove_smallest_unchecked() });

            self.hand_first_defender.remove_existing(card);
            points += card.to_points();
        }

        for _ in 0..count_s_trump {
            let mut available = self.hand_second_defender.and(Cards::of_trump(game_type));
            let card = available
                .and(Cards::OF_ZERO_POINTS)
                .remove_smallest()
                .or_else(|| {
                    available
                        .and(Cards::of_rank(crate::card::Rank::U))
                        .remove_smallest()
                })
                .unwrap_or_else(|| unsafe { available.remove_smallest_unchecked() });

            self.hand_second_defender.remove_existing(card);
            points += card.to_points();
        }

        for suit in [Suit::Clubs, Suit::Diamonds, Suit::Hearts, Suit::Spades] {
            let card_type = CardType::Suit(suit);
            let cards_of_type = Cards::of_card_type(card_type, game_type);

            let mut remaining_suit = remaining_cards.and(cards_of_type);
            while (!self.hand_first_defender.and(cards_of_type).is_empty()
                || self.hand_first_defender.and(trump_cards).is_empty())
                && (!self.hand_second_defender.and(cards_of_type).is_empty()
                    || self.hand_second_defender.and(trump_cards).is_empty())
                && let Some(highest) = remaining_suit.remove_highest_non_null()
                && self.hand_declarer.remove_if_there(highest)
            {
                tricks += 1;
                points += highest.to_points();

                if let Some(lowest) = self
                    .hand_first_defender
                    .remove_lowest_of_type(card_type, game_type)
                {
                    remaining_suit.remove_existing(lowest);
                    points += lowest.to_points();
                } else {
                    count_f += 1;
                }

                if let Some(lowest) = self
                    .hand_second_defender
                    .remove_lowest_of_type(card_type, game_type)
                {
                    remaining_suit.remove_existing(lowest);
                    points += lowest.to_points();
                } else {
                    count_s += 1;
                }
            }
        }

        'outer: for _ in 0..count_f {
            for rank in Rank::BY_POINTS {
                if let Some(card) = self
                    .hand_first_defender
                    .and(Cards::of_rank(rank))
                    .remove_smallest()
                {
                    self.hand_first_defender.remove_existing(card);
                    points += card.to_points();
                    continue 'outer;
                }
            }

            unreachable!("no cards left");
        }

        'outer: for _ in 0..count_s {
            for rank in Rank::BY_POINTS {
                if let Some(card) = self
                    .hand_second_defender
                    .and(Cards::of_rank(rank))
                    .remove_smallest()
                {
                    self.hand_second_defender.remove_existing(card);
                    points += card.to_points();
                    continue 'outer;
                }
            }

            unreachable!("no cards left");
        }

        TrickYield::new(points, tricks)
    }

    pub fn quick_bounds(self, game_type: GameType) -> Bounds {
        let lower = match (self.is_trick_in_progress(), self.active_role, game_type) {
            (false, Role::Declarer, GameType::Trump(trump)) => {
                self.lower_bound_non_null_declarer_forehand(trump)
            }
            _ => TrickYield::ZERO_TRICKS,
        };

        //TODO: How many tricks left?
        let cellar = self.cellar();
        let gone_tricks = cellar.len() / 3;
        let cellar_score = YieldSoFar::new(cellar.to_points(), gone_tricks);
        let upper = YieldSoFar::MAX.saturating_sub(cellar_score);
        Bounds::new(lower, upper)
    }

    pub fn remaining_cards_in_hands(self) -> Cards {
        self.hand_declarer
            .or(self.hand_first_defender)
            .or(self.hand_second_defender)
    }

    /// Everything already face down.
    pub fn cellar(self) -> Cards {
        // TODO: Better to remove trick cards individually?
        let result = Cards::ALL
            .without(self.remaining_cards_in_hands())
            .without(self.partial_trick.cards());
        assert_eq!(result.len() % 3, 2);
        result
    }

    //TODO: Return Yield so far so we don't forget?
    pub fn initial(deal: Deal, bidding_winner: BiddingRole) -> Self {
        let &hand_declarer = deal.hand(bidding_winner);
        let &hand_first_defender = deal.hand(bidding_winner.next());
        let &hand_second_defender = deal.hand(bidding_winner.next().next());
        let partial_trick = PartialTrick::EMPTY;
        let active_role = Role::first_active(bidding_winner);

        let result = Self {
            hand_declarer,
            hand_first_defender,
            hand_second_defender,
            partial_trick,
            active_role,
        };
        debug_assert_eq!(deal.skat(), result.cellar());
        result.assert_invariants();
        result
    }

    pub fn yield_from_skat(self) -> YieldSoFar {
        debug_assert!(self.is_initial_situation());
        YieldSoFar::new(self.cellar().to_points(), 0)
    }

    pub fn leaf(active_role: Role) -> Self {
        Self {
            hand_declarer: Cards::EMPTY,
            hand_first_defender: Cards::EMPTY,
            hand_second_defender: Cards::EMPTY,
            partial_trick: PartialTrick::EMPTY,
            active_role,
        }
    }

    pub(crate) fn in_hand_or_yielded(&self) -> Cards {
        self.active_hand_cards()
            .combined_with_disjoint(self.cellar())
    }
}
