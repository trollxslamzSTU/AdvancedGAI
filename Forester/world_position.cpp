#include "forester.h"

using namespace forester;

world_position::world_position(world_coordinate const & coordinate) :
	_chunk(coordinate.chunk()),
	_xy({((static_cast<float>(coordinate.x()) + 0.5f) / static_cast<float>(world_chunk::cells_per_chunk_sqrt)), ((static_cast<float>(coordinate.y()) + 0.5f) / static_cast<float>(world_chunk::cells_per_chunk_sqrt))}) {}

void world_position::move_direction(float const & step, SDL_Point const & direction)
{
	_xy = _xy + (sdl_game::to_fpoint(direction) * sdl_game::to_fpoint(step));

	while (_xy.x < 0)
	{
		_chunk.x -= 1;
		_xy.x += 1;
	}

	while (_xy.x > 1)
	{
		_chunk.x += 1;
		_xy.x -= 1;
	}

	while (_xy.y < 0)
	{
		_chunk.y -= 1;
		_xy.y += 1;
	}

	while (_xy.y > 1)
	{
		_chunk.y += 1;
		_xy.y -= 1;
	}
}

bool world_position::move_toward(float const & step, world_position const & position)
{
	SDL_FPoint direction =
	{
		.x = (position.chunk().x - _chunk.x) + (position.xy().x - _xy.x),
		.y = (position.chunk().y - _chunk.y) + (position.xy().y - _xy.y),
	};

	float const length_squared = (direction.x * direction.x) + (direction.y * direction.y);
	constexpr float const length_tolerance = 0.001f;

	if (length_squared <= length_tolerance)
	{
		return false;
	}

	if (direction.x < 0)
	{
		direction.x = std::floor(direction.x);
	}

	if (direction.y < 0)
	{
		direction.y = std::floor(direction.y);
	}

	if (direction.x > 0)
	{
		direction.x = std::ceil(direction.x);
	}

	if (direction.y > 0)
	{
		direction.y = std::ceil(direction.y);
	}

	move_direction(step, sdl_game::to_point(direction));

	return true;
}
