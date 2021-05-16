#pragma once

#include <cstdint>

#include <stdc/hasher.h>

#include "card.h"
#include "trick.h"

namespace muskat {
class ScoreNew {
private:
	uint8_t m_points;
	uint8_t m_tricks;
public:
	explicit constexpr ScoreNew(uint8_t points, uint8_t tricks) :
		m_points{points},
		m_tricks{tricks}
	{}

	friend auto operator<=>(ScoreNew, ScoreNew) -> bool = default;
	void add_trick(const Trick &trick, GameType game) {
		m_points += to_points(trick, game);
		++m_tricks;
	}
	[[nodiscard]] auto points() const {
		return m_points;
	}
	[[nodiscard]] auto tricks() const {
		return m_tricks;
	}
	void add(ScoreNew other) {
		m_points += other.m_points;
		m_tricks += other.m_tricks;
	}
};

class Score {
private:
	uint8_t m_points;
	uint8_t m_ignore;
public:
	explicit constexpr Score(uint8_t points, uint8_t) :
		m_points{points}, m_ignore{5}
	{}

	//TODO
	explicit constexpr Score() = default;
	
	friend constexpr auto operator<=>(Score, Score) = default;
	
	void add_trick(const Trick &trick, GameType game) {
		m_points += to_points(trick, game);
	}
	[[nodiscard]] auto points() const {
		return m_points;
	}
	[[nodiscard]] auto tricks() const -> uint8_t {
		return m_ignore;
	}
	void add(Score other) {
		m_points += other.m_points;
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

[[nodiscard]] inline auto someone_is_schwarz(Score score) {
	if (score.tricks() == 0) {
		assert(score.points() == 0);
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
			return std::hash<uint8_t>{}(score.points());
		}
	};
} //namespace std