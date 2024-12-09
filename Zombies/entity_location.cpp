#include "zombies.h"

#include <cmath>

using namespace zombies;

sdl_game::circle_shape entity_location::collision() const
{
	constexpr float radius = 24.f;

	return sdl_game::circle_shape(_position, radius);
}

void entity_location::update(SDL_FPoint const & position, SDL_FPoint const & facing_direction)
{
	constexpr float const rotation_speed = 0.01f;

	if (_breadcrumb_cooldown.tick())
	{
		constexpr float const cooldown_duration_seconds = 0.5f;

		_breadcrumb_cooldown.reset(cooldown_duration_seconds);

		_breadcrumbs[_breadcrumb_tail] = _position;
		_breadcrumb_tail = ((_breadcrumb_tail + 1) % _breadcrumbs.size());
	}

	_position = position;
	_desired_orientation = std::atan2(facing_direction.y, facing_direction.x) * static_cast<float>(180.f / M_PI);
	_current_orientation = sdl_game::lerped_angle(_current_orientation, _desired_orientation, rotation_speed);
}
