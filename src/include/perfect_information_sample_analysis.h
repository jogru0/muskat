#pragma once

#include <array>
#include <vector>

#include <stdc/literals.h>
#include <stdc/utility.h>
#include <stdc/mathematics.h>


#include "cards.h"

namespace muskat {

	using PerfectInformationResult = std::array<uint8_t, 32>;
	class PerfectInformationSample {
	private:
		Cards m_playable_cards;
		std::vector<PerfectInformationResult> m_points_for_situations;
	public:
		[[nodiscard]] auto playable_cards() const {
			return m_playable_cards;
		}
		[[nodiscard]] auto points_for_situations() const {
			return m_points_for_situations;
		}

		PerfectInformationSample(
			Cards playable_cards,
			std::vector<PerfectInformationResult> &&points_for_situations
		) :
			m_playable_cards{std::move(playable_cards)},
			m_points_for_situations{std::move(points_for_situations)}
		{
			using namespace stdc::literals;
			assert(!m_playable_cards.empty());
			
			for (const auto &points : m_points_for_situations) {
				for (auto i = 0_z; i < 32; ++i) {
					assert(points[i] <= 121);
					assert((points[i] < 121) == (m_playable_cards.contains(static_cast<Card>(i))));
				}
			}
		}
	};

	[[nodiscard]] inline auto to_sample(
		std::vector<PerfectInformationResult> &&points_for_situations
	) {
		using namespace stdc::literals;
		
		assert(!points_for_situations.empty());
		auto cards = Cards{};
		for (auto i = 0_z; i < 32; ++i) {
			if (points_for_situations.front()[i] != 121) {
				cards.add(static_cast<Card>(i));
			}
		}

		return PerfectInformationSample{
			cards, std::move(points_for_situations)
		};
	}

	template<typename PointsToSummand>
	[[nodiscard]] auto highest_additive_score(
		const PerfectInformationSample &sample,
		PointsToSummand summand,
		Cards cards_to_consider
	) {
		using namespace stdc::literals;
		
		using Score = std::invoke_result_t<PointsToSummand, uint8_t>;

		const auto &points_for_situations = sample.points_for_situations();
		
		auto cards_to_score = std::vector<std::pair<Card, Score>>{};
		cards_to_score.reserve(cards_to_consider.size());
		auto high_score = Score{};
		while (!cards_to_consider.empty()) {
			auto card = cards_to_consider.remove_next();
			assert(sample.playable_cards().contains(card));
			

			auto score = stdc::transform_accumulate(RANGE(points_for_situations), [&] (auto &points){
				return summand(points[static_cast<size_t>(card)]);
			});

			stdc::maximize(high_score, score);
			cards_to_score.emplace_back(card, score);
		}

		auto result = Cards{};
		for (const auto &[c, s] : cards_to_score) {
			if (s == high_score) {
				result.add(c);
			}
		}

		return result;
	}

	template<typename PointsToSummandHead, typename ...PointsToSummandTail>
	[[nodiscard]] auto highest_additive_score(
		const PerfectInformationSample &sample,
		PointsToSummandHead summand_head,
		PointsToSummandTail ...summand_tail
	) {
		
		auto result = highest_additive_score(
			sample,
			summand_head,
			sample.playable_cards()
		);

		(..., (result = highest_additive_score(
			sample,
			summand_tail,
			result
		)));

		return result;
	}

	[[nodiscard]] inline auto highest_expected_winate_declarer(
		const PerfectInformationSample &sample,
		uint8_t threshold = 61
	) {
		return highest_additive_score(
			sample,
			[&](auto points) {
				return static_cast<size_t>(points >= threshold);
			}
		);
	}

	[[nodiscard]] inline auto highest_expected_winate_defender(
		const PerfectInformationSample &sample,
		uint8_t threshold = 61
	) {
		return highest_additive_score(
			sample,
			[&](auto points) {
				return static_cast<size_t>(points < threshold);
			}
		);
	}


} // namespace muskat
