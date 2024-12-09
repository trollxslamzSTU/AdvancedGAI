#include "sdl_game.h"

using namespace sdl_game;

void countdown::reset(float const & duration_seconds)
{
	_remaining_seconds = duration_seconds;
}

bool countdown::tick()
{
	constexpr float const step = 1 / 600.f;

	_remaining_seconds = std::max(0.f, _remaining_seconds - step);

	return _remaining_seconds <= 0;
}

countdown::duration countdown::remaining() const
{
	return std::chrono::round<std::chrono::nanoseconds>(duration(_remaining_seconds));
}

bool countdown::is_zero() const
{
	return remaining() <= duration::zero();
}
