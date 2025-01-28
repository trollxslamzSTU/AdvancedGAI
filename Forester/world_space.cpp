#include "forester.h"

#include <queue>
#include <unordered_set>

using namespace forester;

namespace
{
	template<size_t num_seeds> struct voronoi_noise final
	{
		template<typename Random> voronoi_noise(Random & random, float width, float height) : _width(width), _height(height)
		{
			auto dist_x = std::uniform_real_distribution<float>(0.f, width);
			auto dist_y = std::uniform_real_distribution<float>(0.f, height);

			for (SDL_FPoint & seed : _seeds)
			{
				seed = {dist_x(random), dist_y(random)};
			}
		}

		float operator()(float x, float y) const
		{
			float min_distance = std::numeric_limits<float>::max();

			for (SDL_FPoint const & seed : _seeds)
			{
				float const distance = std::sqrt((x - seed.x) * (x - seed.x) + (y - seed.y) * (y - seed.y));

				min_distance = std::min(min_distance, distance);
			}

			float noise = min_distance / (std::sqrt((_width * _width) + (_height * _height)));

			return noise;
		}

		private:
		std::array<SDL_FPoint, num_seeds> _seeds = {};

		float _width = 0, _height = 0;
	};
}

world_space::pawn_key world_space::assign_pawn()
{
	if (_unassigned_pawns.empty())
	{
		return pawn_key::invalid();
	}

	pawn_key const assigned = _assigned_pawns.emplace(std::move(_unassigned_pawns.back()));

	_unassigned_pawns.pop_back();

	return assigned;
}

world_chunk world_space::fetch_chunk(SDL_Point const & coordinate) const
{
	if (std::optional changed_chunk = has_chunk_changed(coordinate))
	{
		return changed_chunk->first;
	}

	return generate_chunk(coordinate);
}

std::optional<std::pair<world_chunk const &, world_space::timestamp const &>> world_space::has_chunk_changed(SDL_Point const & coordinate) const
{
	saved_chunk_map::const_iterator const saved_coordinate_chunk = _saved_chunks.find(coordinate);

	if (saved_coordinate_chunk == _saved_chunks.end())
	{
		return std::nullopt;
	}

	return saved_coordinate_chunk->second;
}

world_chunk world_space::generate_chunk(SDL_Point const & coordinate) const
{
	auto health_distribution = std::uniform_real_distribution<float>(0.f, 1.f);
	auto min_chance = std::uniform_int_distribution<size_t>(0, 10);
	auto mid_chance = std::uniform_int_distribution<size_t>(0, 5);
	auto max_chance = std::uniform_int_distribution<size_t>(0, 1);
	world_chunk generated_chunk;

	auto random = std::mt19937(local_seed(coordinate));
	auto noise = voronoi_noise<2>(random, world_chunk::cells_per_chunk_sqrt, world_chunk::cells_per_chunk_sqrt);

	auto const roll_object = [&random](std::uniform_int_distribution<size_t> & chance, world_object object)
	{
		return (chance(random) == 0) ? object : world_object_none;
	};

	for (size_t i = 0; i < world_chunk::cells_per_chunk; i += 1)
	{
		world_chunk::cell & cell = generated_chunk[i];

		cell.tile = static_cast<world_tile>(std::round(static_cast<float>(world_tile_max) * noise(
			static_cast<float>(i / world_chunk::cells_per_chunk_sqrt),
			static_cast<float>(i % world_chunk::cells_per_chunk_sqrt))));

		switch (cell.tile)
		{
			case world_tile_dirt:
			{
				cell.object = roll_object(min_chance, world_object_rock);

				if (!cell.object)
				{
					cell.object = roll_object(min_chance, world_object_tree);
				}
			}
			break;

			case world_tile_grass:
			{
				cell.object = roll_object(max_chance, world_object_tree);

				if (!cell.object)
				{
					cell.object = roll_object(min_chance, world_object_rock);
				}
			}
			break;

			case world_tile_sand:
			{
				cell.object = roll_object(min_chance, world_object_rock);
			}
			break;

			case world_tile_snow:
			{
				cell.object = roll_object(min_chance, world_object_tree);

				if (!cell.object)
				{
					cell.object = roll_object(min_chance, world_object_rock);
				}
			}
			break;

			case world_tile_stone:
			{
				cell.object = roll_object(max_chance, world_object_rock);
			}
			break;

			case world_tile_water: break;
			default: sdl_game::unreachable();
		}

		if (cell.object)
		{
			cell.object_health = std::pow(health_distribution(random), 0.5f);
		}
	}

	return generated_chunk;
}

uint32_t world_space::local_seed(SDL_Point const & coordinate) const
{
	constexpr uint32_t const golden_ratio_prime = 0x9E3779B9;
	uint32_t hash = 0;

	hash ^= static_cast<uint32_t>(coordinate.x) * golden_ratio_prime;
	hash ^= (static_cast<uint32_t>(coordinate.y) + 1) * golden_ratio_prime;

	return hash ^ _global_seed;
}

world_coordinate world_space::query_object(world_coordinate const & origin, world_object const & object) const
{
	// HINT: This environment query performs a breadth-first search from the origin - is this the most ideal way to
	// query an environment?
	if (fetch_chunk(origin.chunk())[origin.xy()].object == object)
	{
		return origin;
	}

	constexpr std::array const directions = std::to_array<SDL_Point>(
	{
		{-1, 0},
		{1, 0},
		{0, -1},
		{0, 1},
	});

	std::queue<world_coordinate> pending_cells;
	std::unordered_set<world_coordinate, world_coordinate::hash> visited_cells;

	pending_cells.push(origin);
	visited_cells.insert(origin);

	while (true)
	{
		SDL_assert(!pending_cells.empty());

		world_coordinate current = pending_cells.front();

		pending_cells.pop();

		for (SDL_Point const & direction : directions)
		{
			current = current.with_offset(direction);

			if (visited_cells.contains(current))
			{
				continue;
			}

			if (fetch_chunk(current.chunk())[current.xy()].object == object)
			{
				return current;
			}

			visited_cells.insert(current);
			pending_cells.push(current);
		}
	}
}

void world_space::spawn(world_position const & position)
{
	_unassigned_pawns.push_back(pawn
	{
		.position = position,
	});
}

bool world_space::unassign_pawn(pawn_key const & key)
{
	if (std::optional const unassigned_pawn = _assigned_pawns.pop(key))
	{
		_unassigned_pawns.push_back(*unassigned_pawn);

		return true;
	}

	return false;
}

void world_space::write_chunk(SDL_Point const & coordinate, world_chunk const & chunk)
{
	_saved_chunks.insert_or_assign(coordinate, std::pair(chunk, std::chrono::system_clock::now()));
}
