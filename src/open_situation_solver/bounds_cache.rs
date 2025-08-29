use crate::{
    open_situation_solver::bounds_and_preference::BoundsAndMaybePreference, role::Role,
    situation::OpenSituation,
};
use std::{collections::HashMap, hash::Hash};

pub fn open_situation_reachable_from_to_u32_key(
    reachable_from: OpenSituation,
) -> impl Fn(OpenSituation) -> u32 {
    let mut cellar = reachable_from.cellar();
    //TODO: Faster to prepare cards as Cards?
    let first_defener_card = unsafe { cellar.remove_next_unchecked() };
    let second_defener_card = unsafe { cellar.remove_next_unchecked() };

    move |open_situation| {
        //TODO: Assert it is reachable from (and other invariants like no empy partial trick etc).
        let mut key = open_situation.remaining_cards_in_hands();
        match open_situation.active_role() {
            Role::Declarer => {}
            Role::FirstDefender => key.add_new(first_defener_card),
            Role::SecondDefender => key.add_new(second_defener_card),
        }

        key.detail_to_bits()
    }
}

pub trait OpenSituationSolverCache {
    fn get_copy(&mut self, situation: OpenSituation) -> BoundsAndMaybePreference;
    fn update_existing(&mut self, situation: OpenSituation, updated: BoundsAndMaybePreference);
}

pub struct DebugOpenSituationSolverCache<Key, OpenSituationToKey> {
    open_situation_to_key: OpenSituationToKey,
    // Given a key created from a situation via `situation_to_key`,
    // this might contain hashed bounds for the trick yield.
    // To always get valid cashed values, you MUST NOT calculate
    // bounds of different situations with the same key.
    // Therefore, what subsets of situations can be analyzed with the same instance
    // is dictated by the properties of `situation_to_key`.
    // For that, also note that we don't cache situations with ongoing ticks.
    key_to_bounds: HashMap<Key, (BoundsAndMaybePreference, OpenSituation)>,
}

impl<Key, OpenSituationToKey> DebugOpenSituationSolverCache<Key, OpenSituationToKey> {
    pub fn new(open_situation_to_key: OpenSituationToKey) -> Self {
        Self {
            open_situation_to_key,
            key_to_bounds: HashMap::new(),
        }
    }
}

impl<Key, OpenSituationToKey> OpenSituationSolverCache
    for DebugOpenSituationSolverCache<Key, OpenSituationToKey>
where
    Key: Hash + Eq,
    OpenSituationToKey: Fn(OpenSituation) -> Key,
{
    fn get_copy(&mut self, open_situation: OpenSituation) -> BoundsAndMaybePreference {
        let key = (self.open_situation_to_key)(open_situation);
        let value = self.key_to_bounds.entry(key).or_insert((
            BoundsAndMaybePreference::new(open_situation.quick_bounds(), None),
            open_situation,
        ));

        debug_assert_eq!(value.1, open_situation);
        value.0
    }

    fn update_existing(
        &mut self,
        open_situation: OpenSituation,
        updated: BoundsAndMaybePreference,
    ) {
        let key = (self.open_situation_to_key)(open_situation);
        let existing = self.key_to_bounds.get_mut(&key);

        debug_assert!(existing.is_some());
        let existing = unsafe { existing.unwrap_unchecked() };

        debug_assert_eq!(existing.1, open_situation);
        existing.0 = updated;
    }
}

pub struct FastOpenSituationSolverCache<Key, OpenSituationToKey> {
    open_situation_to_key: OpenSituationToKey,
    // Given a key created from a situation via `situation_to_key`,
    // this might contain hashed bounds for the trick yield.
    // To always get valid cashed values, you MUST NOT calculate
    // bounds of different situations with the same key.
    // Therefore, what subsets of situations can be analyzed with the same instance
    // is dictated by the properties of `situation_to_key`.
    // For that, also note that we don't cache situations with ongoing ticks.
    key_to_bounds: HashMap<Key, BoundsAndMaybePreference>,
}

impl<Key, OpenSituationToKey> FastOpenSituationSolverCache<Key, OpenSituationToKey> {
    pub fn new(open_situation_to_key: OpenSituationToKey) -> Self {
        Self {
            open_situation_to_key,
            key_to_bounds: HashMap::default(),
        }
    }
}

impl<Key, OpenSituationToKey> OpenSituationSolverCache
    for FastOpenSituationSolverCache<Key, OpenSituationToKey>
where
    Key: Hash + Eq,
    OpenSituationToKey: Fn(OpenSituation) -> Key,
{
    fn get_copy(&mut self, open_situation: OpenSituation) -> BoundsAndMaybePreference {
        let key = (self.open_situation_to_key)(open_situation);

        *self
            .key_to_bounds
            .entry(key)
            .or_insert(BoundsAndMaybePreference::new(
                open_situation.quick_bounds(),
                None,
            ))
    }

    fn update_existing(
        &mut self,
        open_situation: OpenSituation,
        updated: BoundsAndMaybePreference,
    ) {
        let key = (self.open_situation_to_key)(open_situation);
        let existing = self.key_to_bounds.get_mut(&key);

        debug_assert!(existing.is_some());
        let existing = unsafe { existing.unwrap_unchecked() };

        *existing = updated;
    }
}
