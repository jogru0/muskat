#pragma once

namespace muskat {
	class AbstractPlayer {
	private:
		using This = AbstractPlayer;
	
	public:
		virtual void inform_about_role(Role) = 0;
		virtual void inform_about_first_position(Position) = 0;
		virtual void inform_about_game(GameType) = 0;
		virtual void inform_about_deal(Cards) = 0;
		virtual void inform_about_skat(Cards) = 0;
		virtual void inform_about_move(Card) = 0;
		virtual auto request_move() -> Card = 0;
		virtual void cheat(const Situation &) {}

	protected:
		AbstractPlayer() = default;
	protected:
		~AbstractPlayer() noexcept = default;
		constexpr AbstractPlayer(const This &) = default;
		constexpr auto operator=(const This &) & -> AbstractPlayer & = default;
		constexpr AbstractPlayer(This &&) noexcept = default;
		constexpr auto operator=(This &&) & noexcept -> AbstractPlayer & = default;
	};
} // namespace muskat
