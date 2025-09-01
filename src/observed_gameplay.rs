use static_assertions::assert_eq_size;

use crate::{
    bidding_role::BiddingRole,
    card::{Card, CardType},
    cards::Cards,
    deal::Deal,
    dist::UnknownCards,
    game_type::GameType,
    monte_carlo::Contract,
    partial_trick::PartialTrick,
    position::Position,
    situation::OpenSituation,
    trick::Trick,
    trick_yield::YieldSoFar,
};

// TODO: Maybe it makes sense to store/expose them as sequence of (Card, BiddingRole, TrickType).
pub struct ObservedPlayedCards {
    done_tricks: Vec<Trick>,
    current_trick: PartialTrick,
    // Can be calculated from the other values, but we cache it.
    // TODO: Assert at different places that this is correct.
    active_role: BiddingRole,
}

impl ObservedPlayedCards {
    pub fn initial() -> Self {
        Self {
            done_tricks: Vec::new(),
            current_trick: PartialTrick::EMPTY,
            active_role: BiddingRole::FIRST_ACTIVE_PLAYER,
        }
    }

    /// Deal has to be possible.
    pub fn to_open_game_state(
        &self,
        deal: Deal,
        bidding_winner: BiddingRole,
        game_type: GameType,
    ) -> OpenGameState {
        let matadors = deal.matadors(bidding_winner, game_type);

        let mut open_situation = OpenSituation::new(deal, bidding_winner);
        let mut yield_so_far = deal.initial_yield();

        for trick in &self.done_tricks {
            for card in [trick.first(), trick.second(), trick.third()] {
                yield_so_far.add_assign(open_situation.play_card(card, game_type));
            }
        }

        if let Some(card) = self.current_trick.first() {
            yield_so_far.add_assign(open_situation.play_card(card, game_type));
            if let Some(card) = self.current_trick.second() {
                yield_so_far.add_assign(open_situation.play_card(card, game_type));
            }
        }

        OpenGameState {
            open_situation,
            yield_so_far,
            matadors,
        }
    }

    pub fn active_role(&self) -> BiddingRole {
        self.active_role
    }

    pub fn observe_play(&mut self, card: Card, game_type: GameType) {
        self.active_role = self.active_role.next();

        if let Some(trick) = self.current_trick.add(card) {
            self.active_role = trick
                .winner_position(game_type)
                .bidding_role(self.active_role);
            self.done_tricks.push(trick);
        }
    }
}

impl CardKnowledge {
    pub fn apply(&mut self, observed: &ObservedPlayedCards, game_type: GameType) {
        let mut first_player = BiddingRole::FIRST_ACTIVE_PLAYER;
        for trick in &observed.done_tricks {
            let first_card = trick.first();
            let trick_type = first_card.card_type(game_type);

            self.learn_about_if_not_everything_of_that_player_already_known(
                first_card,
                first_player,
                trick_type,
                game_type,
            );

            self.learn_about_if_not_everything_of_that_player_already_known(
                trick.second(),
                first_player.next(),
                trick_type,
                game_type,
            );

            self.learn_about_if_not_everything_of_that_player_already_known(
                trick.third(),
                first_player.next().next(),
                trick_type,
                game_type,
            );

            match trick.winner_position(game_type) {
                Position::Forehand => {}
                Position::Middlehand => first_player = first_player.next(),
                Position::Rearhand => first_player = first_player.next().next(),
            }
        }

        if let Some(first_card) = observed.current_trick.first() {
            let trick_type = first_card.card_type(game_type);

            self.learn_about_if_not_everything_of_that_player_already_known(
                first_card,
                first_player,
                trick_type,
                game_type,
            );

            if let Some(second_card) = observed.current_trick.second() {
                self.learn_about_if_not_everything_of_that_player_already_known(
                    second_card,
                    first_player.next(),
                    trick_type,
                    game_type,
                );
            }
        }
    }

    pub fn observed_cards(&self) -> Cards {
        self.observed_cards[0]
            .combined_with_disjoint(self.observed_cards[1])
            .combined_with_disjoint(self.observed_cards[2])
            .combined_with_disjoint(self.observed_cards[3])
    }

    pub fn unknown_cards_slice(&self) -> &[UnknownCards] {
        &self.unknown_cards
    }

    pub fn observed_cards_array(&self) -> [Cards; 4] {
        self.observed_cards
    }
}

pub struct ObservedInitialGameState {
    start_hand: Cards,
    skat_if_known: Option<Cards>,
    revealed_if_known_and_not_own_hand: Option<Cards>,
    game_type: GameType,
    bidding_role: BiddingRole,
    // TODO: Whould this be part of this?
    // Have to come up with what this type represents and that should settle it.
    bidding_winner: BiddingRole,
    contract: Contract,
}

pub struct CardKnowledge {
    unknown_cards: [UnknownCards; 4],
    observed_cards: [Cards; 4],
}

#[derive(Clone, Copy)]
enum UnknownCardPosition {
    Hand(BiddingRole),
    Skat,
}

assert_eq_size!(UnknownCardPosition, u8);

impl UnknownCardPosition {
    // TODO: very unpublic
    fn id(self) -> usize {
        match self {
            UnknownCardPosition::Hand(BiddingRole::FirstReceiver) => 0,
            UnknownCardPosition::Hand(BiddingRole::FirstCaller) => 1,
            UnknownCardPosition::Hand(BiddingRole::SecondCaller) => 2,
            UnknownCardPosition::Skat => 3,
        }
    }
}

impl CardKnowledge {
    const NO_KNOWLEDGE: Self = Self {
        unknown_cards: [
            UnknownCards::UNKNOWN_HAND,
            UnknownCards::UNKNOWN_HAND,
            UnknownCards::UNKNOWN_HAND,
            UnknownCards::UNKNOWN_SKAT,
        ],
        observed_cards: [Cards::EMPTY; 4],
    };

    fn learn_about_where_the_card_initially_was_without_any_further_inferences(
        &mut self,
        card: Card,
        position: UnknownCardPosition,
    ) {
        let id = position.id();
        self.observed_cards[id].add_new(card);
        self.unknown_cards[id].remove(1);
    }

    pub fn learn_about_if_not_everything_of_that_player_already_known(
        &mut self,
        card: Card,
        bidding_role_of_player_with_this_card: BiddingRole,
        trick_type: CardType,
        game_type: GameType,
    ) {
        let position = UnknownCardPosition::Hand(bidding_role_of_player_with_this_card);

        if self.unknown_cards[position.id()].is_empty() {
            debug_assert!(self.observed_cards[position.id()].contains(card));
            return;
        }

        self.learn_about_where_the_card_initially_was_without_any_further_inferences(
            card, position,
        );

        if card.card_type(game_type) != trick_type {
            self.unknown_cards[position.id()].retrict(trick_type);
        }
    }

    fn initial(
        hand: Cards,
        skat_if_known: Option<Cards>,
        revealed_if_known_and_not_own_hand: Option<Cards>,
        bidding_role: BiddingRole,
        bidding_winner: BiddingRole,
    ) -> Self {
        let mut result = Self::NO_KNOWLEDGE;

        debug_assert_eq!(hand.len(), 10);
        for card in hand {
            result.learn_about_where_the_card_initially_was_without_any_further_inferences(
                card,
                UnknownCardPosition::Hand(bidding_role),
            );
        }

        if let Some(skat) = skat_if_known {
            debug_assert_eq!(skat.len(), 2);
            for card in skat {
                result.learn_about_where_the_card_initially_was_without_any_further_inferences(
                    card,
                    UnknownCardPosition::Skat,
                );
            }
        }

        if let Some(revealed) = revealed_if_known_and_not_own_hand {
            debug_assert_eq!(revealed.len(), 10);
            debug_assert_ne!(bidding_role, bidding_winner);

            for card in revealed {
                result.learn_about_where_the_card_initially_was_without_any_further_inferences(
                    card,
                    UnknownCardPosition::Hand(bidding_winner),
                );
            }
        }

        result
    }

    pub fn from_observation(
        initial: &ObservedInitialGameState,
        tricks: &ObservedPlayedCards,
    ) -> Self {
        let mut result = Self::initial(
            initial.start_hand,
            initial.skat_if_known,
            initial.revealed_if_known_and_not_own_hand,
            initial.bidding_role,
            initial.bidding_winner,
        );
        result.apply(tricks, initial.game_type);
        result
    }
}

impl ObservedInitialGameState {
    pub fn bidding_role(&self) -> BiddingRole {
        self.bidding_role
    }

    pub fn new(
        start_hand: Cards,
        skat_if_known: Option<Cards>,
        revealed_if_known_and_not_own_hand: Option<Cards>,
        game_type: GameType,
        bidding_role: BiddingRole,
        bidding_winner: BiddingRole,
        contract: Contract,
    ) -> Self {
        debug_assert_eq!(start_hand.len(), 10);
        if let Some(skat) = skat_if_known {
            debug_assert_eq!(skat.len(), 2);
        }

        if let Some(revealed) = revealed_if_known_and_not_own_hand {
            debug_assert_eq!(revealed.len(), 10);
            debug_assert!(revealed.and(start_hand).is_empty());
        }

        let is_delcarer = bidding_role == bidding_winner;

        debug_assert!(skat_if_known.is_some() == (is_delcarer && !contract.announcement().hand()));

        debug_assert!(
            revealed_if_known_and_not_own_hand.is_some()
                == (!is_delcarer && contract.announcement().ouvert())
        );

        Self {
            start_hand,
            skat_if_known,
            revealed_if_known_and_not_own_hand,
            game_type,
            bidding_role,
            bidding_winner,
            contract,
        }
    }

    pub fn game_type(&self) -> GameType {
        self.game_type
    }

    pub fn bidding_winner(&self) -> BiddingRole {
        self.bidding_winner
    }

    pub fn contract(&self) -> Contract {
        self.contract
    }
}

pub struct OpenGameState {
    pub open_situation: OpenSituation,
    pub yield_so_far: YieldSoFar,
    pub matadors: Option<u8>,
}

#[cfg(test)]
mod tests {
    use std::fs::File;

    use anyhow::Ok;

    use crate::{
        dist::UniformPossibleDealsFromObservedGameplay,
        dto::Dto,
        observed_gameplay::{CardKnowledge, ObservedPlayedCards},
    };

    #[test]
    fn test_deserialize_sl003() -> Result<(), anyhow::Error> {
        let result: Dto = serde_json::from_reader(File::open("res/sl003.json")?)?;

        let observed_initial = result.pre_game_observations()?;

        let card_knowledge =
            CardKnowledge::from_observation(&observed_initial, &ObservedPlayedCards::initial());

        let dist = UniformPossibleDealsFromObservedGameplay::new(
            &card_knowledge,
            observed_initial.game_type,
        );

        assert_eq!(dist.number_of_color_distributions(), 140);
        assert_eq!(dist.number_of_possibilities(), 184756);

        Ok(())
    }
}
