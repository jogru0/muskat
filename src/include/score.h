#pragma once

#include <cstdint>

#include <stdc/hasher.h>

#include "card.h"
#include "trick.h"

namespace muskat {

class Score {
private:
	uint8_t m_points;
	uint8_t m_tricks;
public:
	[[nodiscard]] constexpr auto makes_probably_sense() const -> bool {
		if (121 <= m_points) {
			return m_tricks == 0;
		}
		if (m_points == 119) {
			return false;
		}
		if (m_points == 1) {
			return false;
		}
		if (m_tricks == 10) {
			return 98 <= m_points;
		}
		if (m_tricks == 0) {
			return m_points <= 22;
		}
		return true;

	}

	explicit constexpr Score() = default;

	explicit constexpr Score(uint8_t points, uint8_t tricks) :
		m_points{points},
		m_tricks{tricks}
	{
		// assert(makes_probably_sense());
	}

	friend constexpr auto operator<=>(Score, Score) = default;
	void add_trick(Trick trick) {
		m_points += to_points(trick);
		++m_tricks;
		assert(makes_probably_sense());
	}
	[[nodiscard]] auto points() const {
		return m_points;
	}
	[[nodiscard]] auto tricks() const {
		return m_tricks;
	}
	void add(Score other) {
		m_points += other.m_points;
		m_tricks += other.m_tricks;

		assert(makes_probably_sense());
	}
};

[[nodiscard]] inline auto required_beyond_to_reach(Score supply, Score target) {
	auto required_points = target.points() <= supply.points()
		? uint8_t{}
		: static_cast<uint8_t>(target.points() - supply.points());
	auto required_tricks = target.tricks() <= supply.tricks()
		? uint8_t{}
		: static_cast<uint8_t>(target.tricks() - supply.tricks());
	
	return Score{required_points, required_tricks};
}

//When this is called, we should already have added all points together, including the skat.
[[nodiscard]] inline auto someone_is_schwarz(Score score) {
	if (score.tricks() == 0) {
		assert(score.points() <= 22);
		return true;
	}
	if (score.tricks() == 10) {
		assert(score.points() == 120);
		return true;
	}

	return false;
}

[[nodiscard]] inline auto someone_is_schneider(Score score) {
	return score.points() <= 30 || 90 <= score.points();
}

} // namespace muskat


namespace std {
	template<>
	struct hash<muskat::Score> {
		[[nodiscard]] auto operator()(muskat::Score score) const
			-> size_t
		{
			// return stdc::GeneralHasher{}(score.points(), score.tricks());
			return stdc::GeneralHasher{}(score.points(), score.tricks());
		}
	};
} //namespace std