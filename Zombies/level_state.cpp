#include "zombies.h"

using namespace zombies;

void level_state::set_wall(SDL_Point const & point, std::optional<level_tile> const & wall_tile)
{
    if (wall_tile)
    {
        _walls.insert_or_assign(point, *wall_tile);
    }
    else
    {
        _walls.erase(point);
    }
}

void level_state::set_floor(SDL_Point const & point, std::optional<level_tile> const & floor_tile)
{
    if (floor_tile)
    {
        _floor.insert_or_assign(point, *floor_tile);
    }
    else
    {
        _floor.erase(point);
    }
}
