#include "forester.h"

using namespace forester;

size_t world_coordinate::hash::operator()(world_coordinate const & coordinate) const
{
	size_t const chunk_hash = std::hash<int>()(coordinate._chunk.x) ^ (std::hash<int>()(coordinate._chunk.y) << 1);
	size_t const xy_hash = std::hash<uint32_t>()(coordinate.x()) ^ (std::hash<uint32_t>()(coordinate.y()) << 1);

	return chunk_hash ^ (xy_hash << 1);
}

world_coordinate::world_coordinate(world_position const& position) : _chunk(position.chunk())
{
	SDL_FPoint coordinate = position.xy();


	coordinate.x *= world_chunk::cells_per_chunk_sqrt;
	coordinate.y *= world_chunk::cells_per_chunk_sqrt;


	coordinate.x = std::floor(coordinate.x);
	coordinate.y = std::floor(coordinate.y);


	_xy = { static_cast<uint32_t>(coordinate.x), static_cast<uint32_t>(coordinate.y) };
}

world_coordinate world_coordinate::with_offset(SDL_Point const & offset) const
{
	uint32_t const absolute_x = x() + offset.x;
	uint32_t const absolute_y = y() + offset.y;
	world_coordinate coordinate;

	coordinate._chunk = _chunk + (SDL_Point(absolute_x, absolute_y) / sdl_game::to_point(world_chunk::cells_per_chunk_sqrt));

	coordinate._xy =
	{
		static_cast<uint32_t>(absolute_x % world_chunk::cells_per_chunk_sqrt),
		static_cast<uint32_t>(absolute_y % world_chunk::cells_per_chunk_sqrt),
	};

	return coordinate;
}
