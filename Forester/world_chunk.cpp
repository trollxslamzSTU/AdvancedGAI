#include "forester.h"

using namespace forester;

world_chunk::cell & world_chunk::operator[](std::pair<uint32_t, uint32_t> const & xy)
{
	auto const & [x, y] = xy;

	return _cells[(y * cells_per_chunk_sqrt) + x];
}

world_chunk::cell const & world_chunk::operator[](std::pair<uint32_t, uint32_t> const & xy) const
{
	auto const & [x, y] = xy;

	return _cells[(y * cells_per_chunk_sqrt) + x];
}

