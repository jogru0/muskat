use static_assertions::assert_eq_size;

use crate::{
    bidding_role::BiddingRole,
    bounds::Bounds,
    card::Card,
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
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
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
        self.cellar().len() == 2
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

    //Returns what the declarer yields with this move.
    pub fn play_card(&mut self, card: Card, game_type: GameType) -> TrickYield {
        debug_assert!(self.next_possible_plays(game_type).contains(card));
        let hand = self.active_hand_cards_mut();
        hand.remove(card);

        self.active_role = self.active_role.next();

        let Some(trick) = self.partial_trick.add(card) else {
            self.assert_invariants();
            return TrickYield::ZERO_TRICKS;
        };

        let position = trick.winner_position(game_type);

        //Active role currently is Vorhand.
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

    pub fn quick_bounds(self) -> Bounds {
        let lower = TrickYield::ZERO_TRICKS;

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

    pub fn new(deal: Deal, bidding_winner: BiddingRole) -> Self {
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
}
