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
    power::CardPower,
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

    pub const fn hand_cards_of(&self, role: Role) -> &Cards {
        match role {
            Role::Declarer => &self.hand_declarer,
            Role::FirstDefender => &self.hand_first_defender,
            Role::SecondDefender => &self.hand_second_defender,
        }
    }

    const fn hand_cards_of_mut(&mut self, role: Role) -> &mut Cards {
        match role {
            Role::Declarer => &mut self.hand_declarer,
            Role::FirstDefender => &mut self.hand_first_defender,
            Role::SecondDefender => &mut self.hand_second_defender,
        }
    }

    const fn active_hand_cards_mut(&mut self) -> &mut Cards {
        self.hand_cards_of_mut(self.active_role)
    }

    const fn active_hand_cards(&self) -> &Cards {
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

    // TODO: For upper bound, can remove points from opposing matadors. Similar for defender forehand.
    fn lower_bound_non_null_declarer_forehand(mut self, trump: CardType) -> TrickYield {
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

    fn upper_bound_non_null_defender_forehand(mut self, trump: CardType) -> TrickYield {
        debug_assert_ne!(self.active_role, Role::Declarer);
        debug_assert!(matches!(self.partial_trick, PartialTrick::EMPTY));

        let cellar = self.cellar();
        let gone_tricks = cellar.len() / 3;
        let cellar_score = YieldSoFar::new(cellar.to_points(), gone_tricks);
        let naive_upper = YieldSoFar::MAX.saturating_sub(cellar_score);

        let game_type = GameType::Trump(trump);
        let trump_cards = Cards::of_card_type(CardType::Trump, game_type);

        let mut count = 0;

        let mut points = CardPoints(0);
        let mut tricks = 0;

        let mut copy_of_declarer_trump_for_points = self.hand_declarer.and(trump_cards);

        // TODO: Can it happen that we fall back from a higher to a lower rule? If not, restructure.
        'outer: loop {
            let remaining_cards = self
                .hand_declarer
                .combined_with_disjoint(self.hand_first_defender)
                .combined_with_disjoint(self.hand_second_defender);

            // Rule 1
            if !self.active_hand_cards().and(trump_cards).is_empty()
                && let matador = remaining_cards
                    .and(trump_cards)
                    .highest_non_null()
                    .expect("trump in active hand")
                && let Some((matador_defender, other_defender)) =
                    if self.hand_cards_of(Role::FirstDefender).contains(matador) {
                        Some((Role::FirstDefender, Role::SecondDefender))
                    } else if self.hand_cards_of(Role::SecondDefender).contains(matador) {
                        Some((Role::SecondDefender, Role::FirstDefender))
                    } else {
                        None
                    }
            {
                self.hand_cards_of_mut(matador_defender)
                    .remove_existing(matador);
                points += matador.to_points();

                let other_defender_card =
                    if self.hand_cards_of(other_defender).overlaps(trump_cards) {
                        self.hand_cards_of(other_defender)
                            .and(trump_cards)
                            .highest_non_null()
                            .expect("has trump cards left")
                    } else {
                        self.hand_cards_of(other_defender)
                            .highest_non_null()
                            .expect("has cards left")
                    };
                self.hand_cards_of_mut(other_defender)
                    .remove_existing(other_defender_card);
                points += other_defender_card.to_points();

                if let Some(_trump_declarer) =
                    self.hand_declarer.remove_lowest_of_cards(trump_cards)
                {
                    points += copy_of_declarer_trump_for_points
                        .remove_lowest_value()
                        .to_points();
                } else {
                    count += 1;
                }

                tricks += 1;
                self.active_role = matador_defender;
                continue 'outer;
            }

            let partner = self.active_role.other_defender();

            // Rule 2
            'suit: for suit in [Suit::Clubs, Suit::Diamonds, Suit::Hearts, Suit::Spades] {
                let suit_cards = Cards::of_card_type(CardType::Suit(suit), game_type);

                if self.active_hand_cards().and(suit_cards).is_empty()
                    || (self.hand_declarer.and(suit_cards).len() <= count
                        && self.hand_declarer.overlaps(trump_cards))
                {
                    continue 'suit;
                }

                let highest = remaining_cards
                    .and(suit_cards)
                    .highest_non_null()
                    .expect("suit in active hand");

                let declarer_has_highest = self.hand_declarer.contains(highest);

                if declarer_has_highest
                    && (self.hand_cards_of(partner).overlaps(suit_cards)
                        || self.hand_cards_of(partner).and(trump_cards).is_empty())
                {
                    continue 'suit;
                }

                let card_active = self
                    .active_hand_cards_mut()
                    .remove_highest_of_cards(suit_cards)
                    .expect("active player has suit");
                points += card_active.to_points();

                match self.hand_declarer.remove_lowest_of_cards(suit_cards) {
                    Some(card) => {
                        points += card.to_points();
                    }
                    None => {
                        count += 1;
                    }
                }

                let card_partner = self
                    .hand_cards_of(partner)
                    .clone()
                    .remove_highest_of_cards(suit_cards)
                    .unwrap_or_else(|| {
                        if declarer_has_highest {
                            self.hand_cards_of(partner).and(trump_cards).highest_value()
                        } else {
                            self.hand_cards_of(partner).highest_value()
                        }
                    });
                self.hand_cards_of_mut(partner)
                    .remove_existing(card_partner);
                points += card_partner.to_points();

                tricks += 1;
                if CardPower::of(card_active, CardType::Suit(suit), game_type)
                    < CardPower::of(card_partner, CardType::Suit(suit), game_type)
                {
                    self.active_role = partner;
                };
                continue 'outer;
            }

            // Rule 3

            // TODO: Same as above for rule 1? Actually, since rule 1 didn't happen, if matador is
            // not with declarer, it is with partner, no? Se debug_assert below.
            // TODO: Would be enough if partner has higher trump than declarer, and plays highest value one with this property.
            let Some(matador) = remaining_cards.and(trump_cards).highest_non_null() else {
                break 'outer;
            };

            if !self.hand_cards_of(partner).contains(matador) {
                debug_assert!(self.hand_declarer.contains(matador));
                break;
            }

            // TODO: Order of suits? Same question for rule 2, probably ...
            'suit: for suit in [Suit::Clubs, Suit::Diamonds, Suit::Hearts, Suit::Spades] {
                let suit_cards = Cards::of_card_type(CardType::Suit(suit), game_type);

                if self.active_hand_cards().and(suit_cards).is_empty()
                    || self.hand_cards_of(partner).overlaps(suit_cards)
                {
                    continue 'suit;
                }

                let card = self.active_hand_cards().and(suit_cards).highest_value();
                self.active_hand_cards_mut().remove_existing(card);
                points += card.to_points();

                self.hand_cards_of_mut(partner).remove_existing(matador);
                points += matador.to_points();

                count += 1;

                tricks += 1;
                self.active_role = partner;
                continue 'outer;
            }

            break 'outer;
        }

        let mut remaining_cards_for_points = self
            .hand_declarer
            .without(trump_cards)
            .or(copy_of_declarer_trump_for_points);

        for _ in 0..count {
            points += remaining_cards_for_points.remove_lowest_value().to_points();
        }

        debug_assert!(points <= naive_upper.card_points());
        debug_assert!(tricks <= naive_upper.number_of_tricks());
        naive_upper.saturating_sub(TrickYield::new(points, tricks))
    }

    pub fn quick_bounds(self, game_type: GameType) -> Bounds {
        let is_trick_in_progress = self.is_trick_in_progress();

        let lower = match (is_trick_in_progress, self.active_role, game_type) {
            (false, Role::Declarer, GameType::Trump(trump)) => {
                self.lower_bound_non_null_declarer_forehand(trump)
            }
            _ => TrickYield::ZERO_TRICKS,
        };

        let upper = match (is_trick_in_progress, self.active_role, game_type) {
            (false, Role::FirstDefender | Role::SecondDefender, GameType::Trump(trump)) => {
                self.upper_bound_non_null_defender_forehand(trump)
            }
            _ => {
                //TODO: How many tricks left?
                let cellar = self.cellar();
                let gone_tricks = cellar.len() / 3;
                let cellar_score = YieldSoFar::new(cellar.to_points(), gone_tricks);
                YieldSoFar::MAX.saturating_sub(cellar_score)
            }
        };

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
        debug_assert_eq!(result.len() % 3, 2);
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

#[cfg(test)]
mod tests {
    use crate::{
        card::{Card, CardType, Suit},
        card_points::CardPoints,
        cards::Cards,
        partial_trick::PartialTrick,
        role::Role,
        situation::OpenSituation,
        trick_yield::TrickYield,
    };

    #[test]
    fn forcing_defender_play_highest_of_suit_for_rule_2() {
        let open_situation = OpenSituation {
            hand_declarer: Cards::from(&[Card::S7, Card::SZ, Card::HK, Card::E9]),
            hand_first_defender: Cards::from(&[Card::G9, Card::GO, Card::E7, Card::EO]),
            hand_second_defender: Cards::from(&[Card::SK, Card::G8, Card::E8, Card::SU]),
            partial_trick: PartialTrick::EMPTY,
            active_role: Role::FirstDefender,
        };

        debug_assert_eq!(
            open_situation.upper_bound_non_null_defender_forehand(CardType::Suit(Suit::Hearts)),
            TrickYield::new(CardPoints(21), 2)
        );
    }

    #[test]
    fn forcing_defender_count_can_allow_declarer_to_use_trump() {
        let open_situation = OpenSituation {
            hand_declarer: Cards::from(&[Card::SO, Card::H7]),
            hand_first_defender: Cards::from(&[Card::G9, Card::E7]),
            hand_second_defender: Cards::from(&[Card::SK, Card::SU]),
            partial_trick: PartialTrick::EMPTY,
            active_role: Role::FirstDefender,
        };

        debug_assert_eq!(
            open_situation.upper_bound_non_null_defender_forehand(CardType::Suit(Suit::Hearts)),
            TrickYield::new(CardPoints(7), 1)
        );
    }
}
