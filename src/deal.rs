use static_assertions::assert_eq_size;

use crate::{bidding_role::BiddingRole, cards::Cards, deck::Deck};

#[derive(Clone, Copy)]
pub struct Deal {
    first_receiver: Cards,
    first_caller: Cards,
    second_caller: Cards,
    // TODO: Make implicit?
    skat: Cards,
}

assert_eq_size!(Deal, u128);

impl Deal {
    pub fn hand(&self, bidding_role: BiddingRole) -> &Cards {
        match bidding_role {
            BiddingRole::FirstReceiver => &self.first_receiver,
            BiddingRole::FirstCaller => &self.first_caller,
            BiddingRole::SecondCaller => &self.second_caller,
        }
    }

    // TODO: Differentiate between deal and known cards
    // TODO: Maybe have some KnownInformation combining Deal and UnknownCards
    pub fn hand_mut(&mut self, bidding_role: BiddingRole) -> &mut Cards {
        match bidding_role {
            BiddingRole::FirstReceiver => &mut self.first_receiver,
            BiddingRole::FirstCaller => &mut self.first_caller,
            BiddingRole::SecondCaller => &mut self.second_caller,
        }
    }

    pub fn skat(self) -> Cards {
        self.skat
    }

    pub fn new(
        first_receiver: Cards,
        first_caller: Cards,
        second_caller: Cards,
        skat: Cards,
    ) -> Self {
        Self {
            first_receiver,
            first_caller,
            second_caller,
            skat,
        }
    }

    pub fn combined_with_disjoint(self, other: Deal) -> Deal {
        Deal {
            first_receiver: self
                .first_receiver
                .combined_with_dosjoint(other.first_receiver),
            first_caller: self.first_caller.combined_with_dosjoint(other.first_caller),
            second_caller: self
                .second_caller
                .combined_with_dosjoint(other.second_caller),
            skat: self.skat.combined_with_dosjoint(other.skat),
        }
    }

    pub fn cards(&self) -> Cards {
        self.first_caller
            .combined_with_dosjoint(self.second_caller)
            .combined_with_dosjoint(self.first_receiver)
            .combined_with_dosjoint(self.skat)
    }
}

impl Deck {
    pub fn deal(&self) -> Deal {
        let mut first_receiver = Cards::EMPTY;
        let mut first_caller = Cards::EMPTY;
        let mut second_caller = Cards::EMPTY;
        let mut skat = Cards::EMPTY;

        first_receiver.add_new(self[0]);
        first_receiver.add_new(self[1]);
        first_receiver.add_new(self[2]);

        first_caller.add_new(self[3]);
        first_caller.add_new(self[4]);
        first_caller.add_new(self[5]);

        second_caller.add_new(self[6]);
        second_caller.add_new(self[7]);
        second_caller.add_new(self[8]);

        skat.add_new(self[9]);
        skat.add_new(self[10]);

        first_receiver.add_new(self[11]);
        first_receiver.add_new(self[12]);
        first_receiver.add_new(self[13]);
        first_receiver.add_new(self[14]);

        first_caller.add_new(self[15]);
        first_caller.add_new(self[16]);
        first_caller.add_new(self[17]);
        first_caller.add_new(self[18]);

        second_caller.add_new(self[19]);
        second_caller.add_new(self[20]);
        second_caller.add_new(self[21]);
        second_caller.add_new(self[22]);

        first_receiver.add_new(self[23]);
        first_receiver.add_new(self[24]);
        first_receiver.add_new(self[25]);

        first_caller.add_new(self[26]);
        first_caller.add_new(self[27]);
        first_caller.add_new(self[28]);

        second_caller.add_new(self[29]);
        second_caller.add_new(self[30]);
        second_caller.add_new(self[31]);

        // TODO: Assert invariants.
        Deal {
            first_receiver,
            first_caller,
            second_caller,
            skat,
        }
    }
}
