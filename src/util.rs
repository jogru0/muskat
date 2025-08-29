pub fn choose(total: u8, selected: u8) -> usize {
    debug_assert!(total <= 10);
    debug_assert!(selected <= total);

    let selected = selected.min(total - selected);
    let mut numerator = 1;
    let mut denominator = 1;
    for i in 0..selected {
        numerator *= (total - i) as usize;
        denominator *= (i + 1) as usize;
    }
    debug_assert_eq!(numerator % denominator, 0);
    numerator / denominator
}

#[cfg(test)]
mod tests {
    use crate::util::choose;

    #[test]
    fn choose_from_5() {
        assert_eq!(choose(5, 0), 1);
        assert_eq!(choose(5, 1), 5);
        assert_eq!(choose(5, 2), 10);
        assert_eq!(choose(5, 3), 10);
        assert_eq!(choose(5, 4), 5);
        assert_eq!(choose(5, 5), 1);
    }

    #[test]
    fn choose_max() {
        assert_eq!(choose(10, 5), 252);
    }
}
