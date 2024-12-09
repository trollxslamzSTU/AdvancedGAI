#include "sdl_game.h"
#include "slot_map.h"

#include <span>

namespace zombies
{
	struct entity_location final
	{
		entity_location() : entity_location(0, {0, 0}) {}

		explicit entity_location(float const & orientation, SDL_FPoint const & position) :
			_current_orientation(orientation),
			_position(position)
		{
			_breadcrumbs.fill(position);
		}

		std::span<SDL_FPoint const> breadcrumbs() const { return std::span(_breadcrumbs); }

		sdl_game::circle_shape collision() const;

		void update(SDL_FPoint const & position, SDL_FPoint const & facing_direction);

		SDL_FPoint const & position() const { return _position; }

		float const & orientation() const { return _current_orientation; }

		private:
		std::array<SDL_FPoint, 24> _breadcrumbs = {};

		SDL_FPoint _position = {};

		float _current_orientation = 0;

		float _desired_orientation = 0;

		sdl_game::countdown _breadcrumb_cooldown = sdl_game::countdown(1.f);

		uint16_t _breadcrumb_tail = 0;
	};

	enum level_tile
	{
		level_tile_wood,
		level_tile_concrete,
	};

	struct level_state final
	{
		using tile_map = std::unordered_map<SDL_Point, level_tile>;

		void set_wall(SDL_Point const & point, std::optional<level_tile> const & wall_tile);

		void set_floor(SDL_Point const & point, std::optional<level_tile> const & floor_tile);

		tile_map const & floor() const { return _floor; }

		tile_map const & walls() const { return _walls; }

		private:
		tile_map _walls;

		tile_map _floor;
	};
}
