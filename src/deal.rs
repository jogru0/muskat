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
    pub fn hand(self, bidding_role: BiddingRole) -> Cards {
        match bidding_role {
            BiddingRole::FirstReceiver => self.first_receiver,
            BiddingRole::FirstCaller => self.first_caller,
            BiddingRole::SecondCaller => self.second_caller,
        }
    }

    pub fn skat(self) -> Cards {
        self.skat
    }
}

impl Deck {
    pub fn deal(&self) -> Deal {
        let mut first_receiver = Cards::EMPTY;
        let mut first_caller = Cards::EMPTY;
        let mut second_caller = Cards::EMPTY;
        let mut skat = Cards::EMPTY;

        first_receiver.add(self[0]);
        first_receiver.add(self[1]);
        first_receiver.add(self[2]);

        first_caller.add(self[3]);
        first_caller.add(self[4]);
        first_caller.add(self[5]);

        second_caller.add(self[6]);
        second_caller.add(self[7]);
        second_caller.add(self[8]);

        skat.add(self[9]);
        skat.add(self[10]);

        first_receiver.add(self[11]);
        first_receiver.add(self[12]);
        first_receiver.add(self[13]);
        first_receiver.add(self[14]);

        first_caller.add(self[15]);
        first_caller.add(self[16]);
        first_caller.add(self[17]);
        first_caller.add(self[18]);

        second_caller.add(self[19]);
        second_caller.add(self[20]);
        second_caller.add(self[21]);
        second_caller.add(self[22]);

        first_receiver.add(self[23]);
        first_receiver.add(self[24]);
        first_receiver.add(self[25]);

        first_caller.add(self[26]);
        first_caller.add(self[27]);
        first_caller.add(self[28]);

        second_caller.add(self[29]);
        second_caller.add(self[30]);
        second_caller.add(self[31]);

        // TODO: Assert invariants.
        Deal {
            first_receiver,
            first_caller,
            second_caller,
            skat,
        }
    }
}
