#include "zombies.h"
#include <random>
using namespace zombies;

namespace
{
	size_t tile_frame(level_tile const & tile)
	{
		switch (tile)
		{
			case level_tile_wood: return 41;
			case level_tile_concrete: return 357;
			default: sdl_game::unreachable();
		}
	}

	enum zombie_archetype
	{
		zombie_archetype_spotter,
		zombie_archetype_healer,
		zombie_archetype_hunter,
	};

	enum zombie_action
	{
		zombie_action_seek_player,
		zombie_action_alerted_to_player,
		zombie_action_idle,
		zombie_action_alerted,
		zombie_action_need_healing,
	};

	struct zombie_actor final
	{
		entity_location location;

		float health = 1.f;

		zombie_archetype archetype = zombie_archetype_hunter;
		Uint32 breadcrumb_cooldown = 0;
		bool is_alerted = false;
		bool is_alerted_to_player = false;
		sdl_game::countdown damage_flash;
		SDL_FPoint zombie_target;
		SDL_FPoint breadcrumb_target;
		SDL_FPoint healer_target;
		bool first_run = true;
		Uint32 breadcrumb_timeout = 0;
		Uint32 wander_cooldown = 0;
		Uint32 lineOfSightCheck = 0;
	};

	std::vector<SDL_FPoint> zombie_sightline(zombie_actor const & zombie)
	{
		float range = 0;
		std::vector<SDL_FPoint> sightlines;
		switch (zombie.archetype)
		{
			case zombie_archetype_hunter:
			{
				range = 128.f;
			}
			break;

			case zombie_archetype_spotter:
			{
				range = 256.f;
			}
			break;

			case zombie_archetype_healer:
			{
				range = 128.f;
			}
			break;
		}
		std::vector<float> angles = { -25.0f, -12.5f, 0.0f, 12.5f, 25.0f };
		for (float angle : angles)
		{
			sightlines.push_back(sdl_game::projected_point(zombie.location.position(), zombie.location.orientation() + angle, range));
		}
		return sightlines;
	}
	void heal_near_zombies(zombie_actor& healer_zombie, dod::slot_map<zombie_actor>& zombies, float healing_radius, float healing_amount)
	{
		for (zombie_actor& zombie : zombies)
		{
			
			/*if (&zombie == &healer_zombie)
			{
				auto const healing_zombie_area = sdl_game::circle_shape(zombie.location.position(), 50.f);
				healing_zombie_area.
			}
			else
			{
				auto const zombie_area = sdl_game::circle_shape(zombie.location.position(), 50.f);
			}*/
			for (zombie_actor& zombie : zombies)
			{
				if (&zombie != &healer_zombie)
				{
					float X_loc = zombie.location.position().x - healer_zombie.location.position().x;
					float Y_loc = zombie.location.position().y - healer_zombie.location.position().y;
					float distance_squared = (X_loc * X_loc) + (Y_loc * Y_loc);
					zombie.healer_target = healer_zombie.location.position();
					if (distance_squared <= healing_radius * healing_radius)
					{
						zombie.health = std::min(zombie.health + healing_amount, 1.0f);
					}
				}
			}
		}
	}
	bool is_point_in_area(SDL_FPoint point, SDL_FPoint centre, float radius)
	{
		float dx = point.x - centre.x;
		float dy = point.y - centre.y;
		float squared = ((dx * dx) - (dy * dy));
		return squared <= (radius * radius);
	}
	float utility_seek_player(zombie_actor const& zombie, SDL_FPoint player_position)
	{
		float distance = static_cast<float>(SDL_sqrt((player_position.x - zombie.location.position().x) * (player_position.x - zombie.location.position().x) +
			(player_position.y - zombie.location.position().y) * (player_position.y - zombie.location.position().y)));
		return 1.0f / (distance + 1.0f);
	}
	float utility_alerted(zombie_actor const& zombie)
	{
		if (zombie.is_alerted)
		{
			return 0.3f + zombie.health;
		}
		return 0.0f;
	}
	float utility_alerted_to_player(zombie_actor const& zombie)
	{
		if (zombie.is_alerted_to_player)
		{
			return 0.4f + zombie.health;
		}
		return 0.0f;
	}
	float utility_idle(zombie_actor const& zombie)
	{
		return 0.1f + zombie.health;
	}
	float utility_need_healing(zombie_actor const& zombie)
	{
		return 1.0f - zombie.health;
	}
	SDL_FPoint breadcrumb_location(zombie_actor & zombie, entity_location const & player_location, level_state _level)
	{
		Uint32 current_time = SDL_GetTicks();
		/*if (current_time < zombie.breadcrumb_cooldown)
		{
			return { -1, -1 }; 
			
		}*/
		for (SDL_FPoint const& breadcrumb : player_location.breadcrumbs())
		{
			auto const breadcrumb_area = sdl_game::circle_shape(breadcrumb, 26.f);
			auto sights = zombie_sightline(zombie);
			for (auto const& vision : sights)
			{
				bool clear_path = true;
				for (auto const [coordinate, _] : _level.walls())
				{
					SDL_FRect wall_centre;
					wall_centre.y = (coordinate.y * 64.0f); //tilesize
					wall_centre.x = (coordinate.x * 64.0f);
					wall_centre.h = 64.0f;
					wall_centre.w = 64.0f;
					sdl_game::circle_shape wall_collision = sdl_game::circle_shape(SDL_FPoint(wall_centre.x + 32.f, wall_centre.y + 32.f), 32.f);
					if (wall_collision.has_line_intersection(zombie.location.position(), vision))
					{
						clear_path = false;
						break;
					}
				}
				if (clear_path && breadcrumb_area.has_line_intersection(zombie.location.position(), vision))
				{
					zombie.breadcrumb_cooldown = current_time + 400;
					zombie.breadcrumb_timeout = current_time + 8000;
					return breadcrumb;
				}
				if (zombie.is_alerted)
				{
					zombie.breadcrumb_cooldown = current_time + 400;
					zombie.breadcrumb_timeout = current_time + 8000;
					return breadcrumb;

				}
				
			}
		}
		return { -1, -1 };
	}
	bool see_player_location(zombie_actor const& zombie, entity_location const& player_location, level_state _level)
	{
		auto const player_area = sdl_game::circle_shape(player_location.position(), 24.f);
		auto sights = zombie_sightline(zombie);
		for (auto const& vision : sights)
		{
			bool clear_path = true;
			for (auto const [coordinate, _] : _level.walls())
			{
				SDL_FRect wall_centre;
				wall_centre.y = (coordinate.y * 64.0f); //tilesize
				wall_centre.x = (coordinate.x * 64.0f);
				wall_centre.h = 64.0f;
				wall_centre.w = 64.0f;
				sdl_game::circle_shape wall_collision = sdl_game::circle_shape(SDL_FPoint(wall_centre.x + 32.f, wall_centre.y + 32.f), 32.f);
				if (wall_collision.has_line_intersection(zombie.location.position(), vision))
				{
					clear_path = false;
					break;
				}
			}
			if (clear_path && player_area.has_line_intersection(zombie.location.position(), vision))
			{
				return true;
			}
		}

		return false;
	}
	void broadcast_alert(zombie_actor& zombie, SDL_FPoint alert_position, dod::slot_map<zombie_actor>& zombies, entity_location const& player_location, level_state _level)
	{
		for (zombie_actor& other_zombie : zombies)
		{
			if (&other_zombie != &zombie && other_zombie.archetype != zombie_archetype_healer)
			{
				if (see_player_location(zombie, player_location, _level))
				{
					other_zombie.is_alerted_to_player = true;
				}
				else {
					other_zombie.is_alerted = true;
					other_zombie.breadcrumb_target = alert_position;
					other_zombie.breadcrumb_timeout = SDL_GetTicks() + 8000;
				}
			}
		}
	}
	zombie_action choose_best_action(zombie_actor& zombie, entity_location const& player_location, dod::slot_map<zombie_actor>& zombies, level_state _level)
	{
		float util_seek = utility_seek_player(zombie, player_location.position());
		float util_alerted_player = utility_alerted_to_player(zombie);
		float util_idle = utility_idle(zombie);
		float util_alerted = utility_alerted(zombie);
		float util_need_healing = utility_need_healing(zombie);
		if (zombie.archetype == zombie_archetype_healer)
		{
			return zombie_action_idle;
		}
		if (util_alerted > util_seek && util_alerted > util_idle && util_alerted > util_alerted_player && util_alerted > util_need_healing)
		{
			SDL_FPoint breadcrumb_loc = breadcrumb_location(zombie, player_location, _level);
			broadcast_alert(zombie, breadcrumb_loc, zombies, player_location, _level);
			return zombie_action_alerted;

		}
		/*else if (util_alerted_player > util_seek && util_alerted_player > util_alerted && util_alerted_player > util_idle && util_alerted_player > util_need_healing)
		{
			broadcast_alert(zombie, player_location.position(), zombies, player_location, _level);
			return zombie_action_alerted_to_player;
		}*/
		else if (util_need_healing > util_seek && util_need_healing > util_alerted && util_need_healing > util_idle && util_need_healing > util_alerted_player)
		{
			return zombie_action_need_healing;
		}
		else
		{
			return zombie_action_idle;
		}
	}
	bool is_point_in_wall(SDL_FPoint point, std::vector<SDL_FRect> const& walls)
	{
		SDL_Point newPoint = { static_cast<int>(point.x), static_cast<int>(point.y) };
		for (auto const& wall : walls)
		{
			SDL_Rect rect = {
				static_cast<int>(wall.x),
				static_cast<int>(wall.y),
				static_cast<int>(wall.w),
				static_cast<int>(wall.h)
			};

			if (SDL_PointInRect(&newPoint, &rect))
			{
				return true;
			}
		}
		return false;
	}

	SDL_FPoint get_random_position_in_arena(int arena_width, int arena_height, std::vector<SDL_FRect> const& walls)
	{
		SDL_FPoint position;
		do
		{
			float x = static_cast<float>(rand() % arena_width);
			float y = static_cast<float>(rand() % arena_height);
			position = { x, y };
		} while (is_point_in_wall(position, walls));

		return position;
	}
	SDL_FPoint Seek(SDL_FPoint target, SDL_FPoint position)
	{
		SDL_FPoint velocity = target - position;
		float distance = static_cast<float>(SDL_sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y)));
		if (distance > 0)
		{
			velocity.x /= distance;
			velocity.y /= distance;
		}
		constexpr float const max_speed = 0.1f;
		velocity.x *= max_speed;
		velocity.y *= max_speed;

		return velocity;

	}
	SDL_FPoint Avoid(SDL_FPoint position, zombie_actor const & zombie, level_state _level)
	{
		float avoid_distance = 0;
		SDL_FPoint avoidance = { 0, 0 };
		switch (zombie.archetype)
		{
		case zombie_archetype_hunter:
		{
			avoid_distance = 50.f;
		}
		break;

		case zombie_archetype_spotter:
		{
			avoid_distance = 50.f;
		}
		break;
		}
		for (auto const [coordinate, _] : _level.walls())
		{
			SDL_FRect wall_centre;
			wall_centre.y = (coordinate.y * 64.0f); //tilesize
			wall_centre.x = (coordinate.x * 64.0f);
			wall_centre.h = 64.0f;
			wall_centre.w = 64.0f;

			sdl_game::circle_shape wall_collision = sdl_game::circle_shape(SDL_FPoint(wall_centre.x + 32.f, wall_centre.y + 32.f), 32.f);

			if (wall_collision.is_circle_intersecting(zombie.location.position(), avoid_distance))
			{
				float distance = sdl_game::point_distance(SDL_FPoint(wall_centre.x + 32.f, wall_centre.y + 32.f), zombie.location.position());

				if (distance > 0)
				{
					avoidance.x += distance;
					avoidance.y += distance;
				}
			}

			/*if (SDL_PointInFRect(&zombie.location.position(), &wall_centre))
			{
				SDL_FPoint difference = zombie.location.position() - SDL_FPoint(wall_centre.x, wall_centre.y);
				float distance = static_cast<float>(SDL_sqrt(difference.x * difference.x + difference.y * difference.y));

				if (distance > 0)
				{
					avoidance.x += difference.x / distance;
					avoidance.y += difference.y / distance;
				}
			}*/
		}
		float avoidance_length = static_cast<float>(SDL_sqrt(avoidance.x * avoidance.x + avoidance.y * avoidance.y));
		if (avoidance_length > 0)
		{
			avoidance.x /= avoidance_length;
			avoidance.y /= avoidance_length;
		}
		/*constexpr float avoidance_strength = 0.2f;
		avoidance.x *= avoidance_strength;
		avoidance.y *= avoidance_strength;*/

		return avoidance;
	}
	
	enum gun_type
	{
		gun_type_pistol,
		gun_type_rifle,
		gun_type_silenced,
		gun_type_max,
	};

	struct gun_item final
	{
		SDL_FPoint position = {};

		gun_type type = gun_type_pistol;
	};

	float gun_range(gun_type const & gun)
	{
		switch (gun)
		{
			case gun_type_rifle: return 256.f;
			case gun_type_pistol: return 128.f;
			case gun_type_silenced: return 96.f;
			default: sdl_game::unreachable();
		}
	}

	float gun_damage(gun_type const & gun)
	{
		switch (gun)
		{
			case gun_type_rifle: return 0.25f;
			case gun_type_pistol: return 0.15f;
			case gun_type_silenced: return 0.10f;
			default: sdl_game::unreachable();
		}
	}

	struct game_loop final
	{
		bool on_ready(sdl_game::app_context & context)
		{
			using sdl_game::texture_info;

			constexpr auto const default_texture_filtering = sdl_game::texture_filtering_linear;

			if (!(_marker_texture = context.load_texture("./marker.png", default_texture_filtering)))
			{
				return false;
			}

			{
				texture_info const info = context.query_texture(_marker_texture);

				_marker_size =
				{
					.x = static_cast<float>(info.width),
					.y = static_cast<float>(info.height),
				};
			}

			if (!(_tiles_texture = context.load_texture("./tiles_sheet.png", default_texture_filtering)))
			{
				return false;
			}

			if (!(_survivor_texture = context.load_texture("./survivor_sheet.png", default_texture_filtering)))
			{
				return false;
			}

			{
				texture_info const info = context.query_texture(_survivor_texture);

				_survivor_size =
				{
					.x = static_cast<float>(info.width / survivor_sheet_columns),
					.y = static_cast<float>(info.height / survivor_sheet_rows),
				};
			}

			if (!(_zombies_texture = context.load_texture("./zombies_sheet.png", default_texture_filtering)))
			{
				return false;
			}

			{
				texture_info const info = context.query_texture(_zombies_texture);

				_zombies_size =
				{
					.x = static_cast<float>(info.width / zombies_sheet_columns),
					.y = static_cast<float>(info.height / zombies_sheet_rows),
				};
			}

			if (!(_guns_texture = context.load_texture("./guns_sheet.png", default_texture_filtering)))
			{
				return false;
			}

			{
				texture_info const info = context.query_texture(_guns_texture);

				_guns_size =
				{
					.x = static_cast<float>(info.width / gun_type_max),
					.y = static_cast<float>(info.height),
				};
			}

			if (!(_game_font = context.load_sprite_font("./help_me.ttf", 44)))
			{
				return false;
			}

			int const floor_width = 20;
			int const floor_height = 12;
			int const floor_area = floor_width * floor_height;

			for (int i = 0; i < floor_area; i += 1)
			{
				int const y = i / floor_width;
				int const x = i % floor_width;

				_level.set_floor({x, y}, level_tile_wood);
			}

			_level.set_wall({2, 4}, level_tile_concrete);
			_level.set_wall({2, 3}, level_tile_concrete);
			_level.set_wall({2, 2}, level_tile_concrete);
			_level.set_wall({3, 2}, level_tile_concrete);
			_level.set_wall({4, 2}, level_tile_concrete);
			_level.set_wall({5, 2}, level_tile_concrete);
			_level.set_wall({6, 2}, level_tile_concrete);

			_level.set_wall({17, 4}, level_tile_concrete);
			_level.set_wall({17, 3}, level_tile_concrete);
			_level.set_wall({13, 2}, level_tile_concrete);
			_level.set_wall({14, 2}, level_tile_concrete);
			_level.set_wall({15, 2}, level_tile_concrete);
			_level.set_wall({16, 2}, level_tile_concrete);
			_level.set_wall({17, 2}, level_tile_concrete);

			_level.set_wall({2, 7}, level_tile_concrete);
			_level.set_wall({2, 8}, level_tile_concrete);
			_level.set_wall({2, 9}, level_tile_concrete);
			_level.set_wall({3, 9}, level_tile_concrete);
			_level.set_wall({4, 9}, level_tile_concrete);
			_level.set_wall({5, 9}, level_tile_concrete);
			_level.set_wall({6, 9}, level_tile_concrete);

			_level.set_wall({17, 7}, level_tile_concrete);
			_level.set_wall({17, 8}, level_tile_concrete);
			_level.set_wall({13, 9}, level_tile_concrete);
			_level.set_wall({14, 9}, level_tile_concrete);
			_level.set_wall({15, 9}, level_tile_concrete);
			_level.set_wall({16, 9}, level_tile_concrete);
			_level.set_wall({17, 9}, level_tile_concrete);

			_level.set_wall({8, 5}, level_tile_concrete);
			_level.set_wall({9, 5}, level_tile_concrete);
			_level.set_wall({10, 5}, level_tile_concrete);
			_level.set_wall({11, 5}, level_tile_concrete);
			_level.set_wall({8, 6}, level_tile_concrete);
			_level.set_wall({9, 6}, level_tile_concrete);
			_level.set_wall({10, 6}, level_tile_concrete);
			_level.set_wall({11, 6}, level_tile_concrete);

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {1200, 300}),
				.archetype = zombie_archetype_spotter,
			});

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {1000, 500}),
				.archetype = zombie_archetype_healer,
			});

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {450, 500}),
				.archetype = zombie_archetype_spotter,
			});

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {300, 470}),
				.archetype = zombie_archetype_spotter,
			});

			_guns.emplace(gun_item
			{
				.position = {150, 50},
				.type = gun_type_pistol,
			});

			_guns.emplace(gun_item
			{
				.position = {690, 500},
				.type = gun_type_silenced,
			});

			_player  = entity_location(0,
			{
				.x = 64,
				.y = 64,
			});

			return true;
		}

		void on_update(sdl_game::app_context const & context)
		{
			using sdl_game::circle_shape;
			using sdl_game::projected_point;
			using sdl_game::to_fpoint;
			
			if (_player_health <= 0)
			{
				return;
			}

			if (context.key_buttons().is_pressed(SDL_SCANCODE_H))
			{
				_is_highlights_enabled = !_is_highlights_enabled;
			}

			SDL_Point const key_direction = context.key_axis(
			{
				.x_neg = SDL_SCANCODE_A,
				.x_pos = SDL_SCANCODE_D,
				.y_neg = SDL_SCANCODE_W,
				.y_pos = SDL_SCANCODE_S,
			});

			constexpr float const player_speed = 1.5f;
			SDL_FPoint const movement = to_fpoint(key_direction) * to_fpoint(player_speed);
			SDL_FPoint moved_player_position = _player.position() + movement;
			auto const player_area = circle_shape(moved_player_position, 24.f);
			std::vector<SDL_FRect> walls;
			for (auto const [coordinate, _] : _level.walls())
			{
				SDL_FRect const wall_rect =
				{
					.x = static_cast<float>(coordinate.x * tile_size.x),
					.y = static_cast<float>(coordinate.y * tile_size.y),
					.w = tile_size.x,
					.h = tile_size.y,
				};
				walls.push_back(wall_rect);
				if (player_area.has_rect_intersection(wall_rect))
				{
					moved_player_position = _player.position();

					break;
				}
			}

			if (_player_damaged_cooldown.tick())
			{
				for (zombie_actor & zombie : _zombies)
				{
					if (zombie.location.collision().is_circle_intersecting(player_area.origin(), player_area.radius()))
					{
						constexpr float const damaged_cooldown_seconds = 0.5f;

						_player_health = std::max(0.f, _player_health - 0.1f);

						_player_damaged_cooldown.reset(damaged_cooldown_seconds);
					}
				}
			}

			_player.update(moved_player_position, context.mouse_position() - _player.position());

			for (auto const & [key, gun] : _guns.items())
			{
				SDL_FRect const gun_rect =
				{
					.x = gun.get().position.x,
					.y = gun.get().position.y,
					.w = _guns_size.x,
					.h = _guns_size.y,
				};

				if (player_area.has_rect_intersection(gun_rect))
				{
					if (_player_gun)
					{
						_guns.emplace(gun_item
						{
							.position = projected_point(_player.position(), _player.orientation() - 180.f, 64.f),
							.type = *_player_gun,
						});
					}

					_player_gun = gun.get().type;

					_guns.erase(key);
				}
			}

			for (zombie_actor & zombie : _zombies)
			{
				
				zombie.damage_flash.tick();
				

				zombie_action action = choose_best_action(zombie, _player, _zombies, _level);
				switch (action)
				{
					case zombie_action_seek_player:
					{
						//SDL_FPoint direction = _player.position() - zombie.location.position();
						//float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
						//SDL_FPoint _seekVelocity = Seek(_player.position(), zombie.location.position());
						//SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
						//SDL_FPoint steering = _seekVelocity + _avoidVelocity;
						//float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

						//if (steering_distance > 0)
						//{
						//	steering.x /= steering_distance;
						//	steering.y /= steering_distance;
						//}

						//// Move the zombie towards the player
						//constexpr float const zombie_speed = 0.35f;
						//SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
						//zombie.location.update(moved_zombie_position, direction);
						//if (see_player_location(zombie, _player))
						//{
						//	zombie.is_alerted_to_player = true;
						//}
						//else {
						//	SDL_FPoint breadcrumb_loc = breadcrumb_location(zombie, _player);
						//	if (breadcrumb_loc.x != 0.f && breadcrumb_loc.y != 0.f)
						//	{
						//		zombie.is_alerted = true;
						//		zombie.breadcrumb_target = breadcrumb_loc;
						//	}
						//}
					}
					break;
					//case zombie_action_alerted_to_player:
					//{
					//	Uint32 current_time = SDL_GetTicks();

					//	if (current_time >= zombie.lineOfSightCheck)
					//	{
					//		zombie.lineOfSightCheck = current_time + 5000;
					//		if (!see_player_location(zombie, _player, _level))
					//		{
					//			zombie.is_alerted_to_player = false;
					//			break;
					//		}
					//	}
					//	SDL_FPoint direction = _player.position() - zombie.location.position();
					//	float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
					//	SDL_FPoint _seekVelocity = Seek(_player.position(), zombie.location.position());
					//	SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
					//	SDL_FPoint steering = _seekVelocity + (_avoidVelocity * to_fpoint(1));
					//	float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

					//	if (steering_distance > 0)
					//	{
					//		steering.x /= steering_distance;
					//		steering.y /= steering_distance;
					//	}

					//	// Move the zombie towards the player
					//	constexpr float const zombie_speed = 0.4f;
					//	SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
					//	zombie.location.update(moved_zombie_position, direction);
					//	if (distance < 20.0f)
					//	{
					//		zombie.is_alerted_to_player = false;
					//		
					//	}
					//}
					//break;
					case zombie_action_alerted:
					{
						Uint32 current_time = SDL_GetTicks();
						if (zombie.breadcrumb_target.x == -1 && zombie.breadcrumb_target.y == -1)
						{
							zombie.is_alerted = false;
							zombie.breadcrumb_timeout = 0;
							break;
						}
						if (current_time >= zombie.breadcrumb_timeout)
						{
							zombie.is_alerted = false;
							zombie.breadcrumb_timeout = 0;
							break;
						}
						SDL_FPoint direction = zombie.breadcrumb_target - zombie.location.position();
						float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
						SDL_FPoint _seekVelocity = Seek(zombie.breadcrumb_target, zombie.location.position());
						SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
						SDL_FPoint steering = _seekVelocity + (_avoidVelocity * to_fpoint(1));
						float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

						if (steering_distance > 0)
						{
							steering.x /= steering_distance;
							steering.y /= steering_distance;
						}

						// Move the zombie towards the player
						constexpr float const zombie_speed = 0.3f;
						SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
						zombie.location.update(moved_zombie_position, direction);
						if (distance < 20.0f)
						{
							zombie.is_alerted = false;
							zombie.breadcrumb_target = { -1, -1 };
						}
					}
					break;
					case zombie_action_need_healing:
					{
						SDL_FPoint direction = zombie.healer_target - zombie.location.position();
						float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
						SDL_FPoint _seekVelocity = Seek(zombie.healer_target, zombie.location.position());
						SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
						SDL_FPoint steering = _seekVelocity + (_avoidVelocity * to_fpoint(1));
						float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

						if (steering_distance > 0)
						{
							steering.x /= steering_distance;
							steering.y /= steering_distance;
						}

						// Move the zombie towards the player
						constexpr float const zombie_speed = 0.5f;
						SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
						zombie.location.update(moved_zombie_position, direction);
						
					}
					break;
					case zombie_action_idle:
					{
						if (zombie.archetype == zombie_archetype_healer)
						{
							int game_width = 1280;
							int game_height = 768;
							SDL_FPoint direction = zombie.location.position() - _player.position();
							float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
							SDL_FPoint _seekVelocity = Seek(zombie.location.position() + direction, zombie.location.position());
							SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
							SDL_FPoint steering = _seekVelocity + (_avoidVelocity * to_fpoint(1));
							float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

							if (steering_distance > 0)
							{
								steering.x /= steering_distance;
								steering.y /= steering_distance;
							}

							// Move the zombie towards the player
							constexpr float const zombie_speed = 0.25f;
							SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
							if (moved_zombie_position.x < 0) moved_zombie_position.x = 0;
							if (moved_zombie_position.x > game_width) moved_zombie_position.x = static_cast<float>(game_width);
							if (moved_zombie_position.y < 0) moved_zombie_position.y = 0;
							if (moved_zombie_position.y > game_height) moved_zombie_position.y = static_cast<float>(game_height);
							zombie.location.update(moved_zombie_position, direction);
							heal_near_zombies(zombie, _zombies, 100.0f, 0.05f); 
							break;
						}
						Uint32 current_time = SDL_GetTicks();
						if (zombie.zombie_target.x == 0 && zombie.zombie_target.y == 0 || current_time >= zombie.wander_cooldown)
						{
							zombie.zombie_target = get_random_position_in_arena(20 * 64, 12 * 64, walls);
							zombie.wander_cooldown = current_time + 25000;
						}


						SDL_FPoint direction = zombie.zombie_target - zombie.location.position();
						float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
						SDL_FPoint _seekVelocity = Seek(zombie.zombie_target, zombie.location.position());
						SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
						SDL_FPoint steering = _seekVelocity + (_avoidVelocity * to_fpoint(1));
						float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

						if (steering_distance > 0)
						{
							steering.x /= steering_distance;
							steering.y /= steering_distance;
						}

						// Move the zombie towards the player
						constexpr float const zombie_speed = 0.2f;
						SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
						zombie.location.update(moved_zombie_position, direction);
						
						if (distance < 20.0f)
						{
							zombie.zombie_target = get_random_position_in_arena(20 * 64, 12 * 64, walls);
						}
						if (see_player_location(zombie, _player, _level))
						{
							zombie.is_alerted_to_player = true;
						}
						else {
							SDL_FPoint breadcrumb_loc = breadcrumb_location(zombie, _player, _level);
							if (breadcrumb_loc.x != -1 && breadcrumb_loc.y != -1)
							{
								zombie.is_alerted = true;
								zombie.breadcrumb_target = breadcrumb_loc;
							}
						}
					}
					break;
					default:
					{

					}
					break;
				}



				//if (zombie.is_alerted)
				//{
				//	SDL_FPoint direction = _player.position() - zombie.location.position();
				//	float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
				//	SDL_FPoint _seekVelocity = Seek(_player.position(), zombie.location.position());
				//	SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
				//	SDL_FPoint steering = _seekVelocity + _avoidVelocity;
				//	float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

				//	if (steering_distance > 0)
				//	{
				//		steering.x /= steering_distance;
				//		steering.y /= steering_distance;
				//	}

				//	// Move the zombie towards the player
				//	constexpr float const zombie_speed = 0.35f;
				//	SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
				//	zombie.location.update(moved_zombie_position, direction);
				//}
				//else
				//{
				//	
				//	SDL_FPoint direction = _player.position() - zombie.location.position();
				//	float distance = static_cast<float>(SDL_sqrt(direction.x * direction.x + direction.y * direction.y));
				//	SDL_FPoint _seekVelocity = Seek(_player.position(), zombie.location.position());
				//	SDL_FPoint _avoidVelocity = Avoid(zombie.location.position(), zombie, _level);
				//	SDL_FPoint steering = _seekVelocity + _avoidVelocity;
				//	float steering_distance = static_cast<float>(SDL_sqrt((steering.x * steering.x) + (steering.y * steering.y)));

				//	if (steering_distance > 0)
				//	{
				//		steering.x /= steering_distance;
				//		steering.y /= steering_distance;
				//	}

				//	// Move the zombie towards the player
				//	constexpr float const zombie_speed = 0.35f;
				//	SDL_FPoint moved_zombie_position = zombie.location.position() + steering * to_fpoint(zombie_speed);
				//	zombie.location.update(moved_zombie_position, direction);
				//}
				
			}

			if (_player_gun && context.mouse_buttons().is_pressed(SDL_BUTTON_LEFT))
			{
				for (zombie_actor & zombie : _zombies)
				{
					SDL_FPoint const firing_cast =
						projected_point(_player.position(), _player.orientation(), gun_range(*_player_gun));

					if (zombie.location.collision().has_line_intersection(_player.position(), firing_cast))
					{
						constexpr float const flash_duration_seconds = 0.05f;

						zombie.damage_flash.reset(flash_duration_seconds);

						zombie.health = std::max(0.f, zombie.health - gun_damage(*_player_gun));
					}
				}
			}

			for (auto const & [key, zombie] : _zombies.items())
			{
				if (zombie.get().health <= 0.f)
				{
					_zombies.erase(key);
				}
			}
		}

		void on_render(sdl_game::app_context & context)
		{
			using sdl_game::greyscale;
			using sdl_game::rgb;
			using sdl_game::text_metrics;
			using sdl_game::to_fpoint;

			context.render_clear();

			if (_player_health <= 0)
			{
				sdl_game::texture_info const renderer_info = context.query_renderer();
				std::string_view const message = "Game Over";
				text_metrics const metrics = text_metrics::measure_text(_game_font, message);

				SDL_FPoint const message_position =
				{
					.x = (renderer_info.width / 2.f) - (metrics.width / 2.f),
					.y = (renderer_info.height / 2.f) - (metrics.height / 2.f),
				};

				context.render_font(_game_font, message_position, message,
				{
					.fill_color = rgb(0.75, 0, 0),
				});
			}
			else
			{
				constexpr SDL_Color const wall_tint = greyscale(0.33f);

				for (auto const & [coordinate, tile] : _level.floor())
				{
					context.render_sheet(_tiles_texture, to_fpoint(coordinate) * tile_size,
					{
						.rows = tiles_sheet_rows,
						.columns = tiles_sheet_columns,
						.frame = tile_frame(tile),
					});
				}

				for (auto const & [coordinate, tile] : _level.walls())
				{
					context.render_sheet(_tiles_texture, to_fpoint(coordinate) * tile_size,
					{
						.rows = tiles_sheet_rows,
						.columns = tiles_sheet_columns,
						.frame = tile_frame(tile),
						.tint_color = wall_tint,
					});
				}

				if (_is_highlights_enabled)
				{
					render_location(context, _player, rgb(0, 1, 0));

					for (zombie_actor const & zombie : _zombies)
					{
						constexpr SDL_Color const zombie_color = rgb(1, 0, 0);

						render_location(context, zombie.location, zombie_color);
						render_sightline(context, zombie, zombie_color);
					}
				}

				for (gun_item const & gun : _guns)
				{
					context.render_sheet(_guns_texture, gun.position,
					{
						.columns = gun_type_max,
						.frame = static_cast<uint16_t>(gun.type),
					});
				}

				SDL_FPoint const zombies_center = _zombies_size / to_fpoint(2);

				for (zombie_actor const & zombie : _zombies)
				{
					context.render_sheet(_zombies_texture, zombie.location.position() - zombies_center,
					{
						.rotation = zombie.location.orientation(),
						.rows = zombies_sheet_rows,
						.columns = zombies_sheet_columns,
						.frame = static_cast<uint32_t>((zombies_sheet_columns * zombie.archetype) + zombie.is_alerted),
						.tint_color = zombie.damage_flash.is_zero() ? greyscale(1) : rgb(1, 0, 0),
					});
				}

				SDL_FPoint const survivor_center = _survivor_size / to_fpoint(2);
				SDL_FPoint const survivor_origin = _player.position() - survivor_center;

				if (_player_gun)
				{
					context.render_sheet(_survivor_texture, survivor_origin,
					{
						.rotation = _player.orientation(),
						.rows = survivor_sheet_rows,
						.columns = survivor_sheet_columns,
						.frame = 2,
						.tint_color = _player_damaged_cooldown.is_zero() ? greyscale(1) : rgb(1, 0, 0),
					});

					context.render_sheet(_survivor_texture, survivor_origin,
					{
						.rotation = _player.orientation(),
						.rows = survivor_sheet_rows,
						.columns = survivor_sheet_columns,
						.frame = static_cast<uint16_t>(survivor_sheet_columns + static_cast<uint16_t>(*_player_gun)),
					});
				}
				else
				{
					context.render_sheet(_survivor_texture, survivor_origin,
					{
						.rotation = _player.orientation(),
						.rows = survivor_sheet_rows,
						.columns = survivor_sheet_columns,
						.frame = 0,
					});
				}

				_text_buffer.clear();
				std::format_to(std::back_inserter(_text_buffer), "hp: {}", std::floor(_player_health * 100.f));

				context.render_font(_game_font, {1100, 15}, _text_buffer,
				{
					.fill_color = rgb(0.75, 0, 0),
				});
			}
		}

		private:
		static constexpr SDL_FPoint const tile_size = sdl_game::to_fpoint(64.f);

		static constexpr uint16_t tiles_sheet_columns = 27;

		static constexpr uint16_t tiles_sheet_rows = 20;

		static constexpr uint16_t zombies_sheet_columns = 2;

		static constexpr uint16_t zombies_sheet_rows = 2;

		static constexpr uint16_t survivor_sheet_columns = 3;

		static constexpr uint16_t survivor_sheet_rows = 2;

		std::string _text_buffer;

		sdl_game::countdown _player_damaged_cooldown;

		sdl_game::sprite_font _game_font;

		dod::slot_map<zombie_actor> _zombies;

		dod::slot_map<gun_item> _guns;

		std::shared_ptr<SDL_Texture> _tiles_texture;

		std::shared_ptr<SDL_Texture> _survivor_texture;

		SDL_FPoint _survivor_size = {};

		std::shared_ptr<SDL_Texture> _zombies_texture;

		SDL_FPoint _zombies_size = {};

		std::shared_ptr<SDL_Texture> _guns_texture;

		SDL_FPoint _guns_size = {};

		std::shared_ptr<SDL_Texture> _marker_texture;

		SDL_FPoint _marker_size = {};

		entity_location _player;

		std::optional<gun_type> _player_gun;

		float _player_health = 1.f;

		level_state _level;

		bool _is_highlights_enabled = false;

		static void render_sightline(sdl_game::app_context & context, zombie_actor const & zombie, SDL_Color const & tint)
		{
			auto sights = zombie_sightline(zombie);
			for (auto const& vision : sights)
			{
				context.render_line(tint, zombie.location.position(), vision);
			}
		}

		void render_location(sdl_game::app_context & context, entity_location const & location, SDL_Color const & tint)
		{
			SDL_FPoint const marker_center = _marker_size / sdl_game::to_fpoint(2);

			for (SDL_FPoint const & breadcrumb : location.breadcrumbs())
			{
				context.render_sprite(_marker_texture, breadcrumb - marker_center,
				{
					.tint_color = tint,
				});
			}
		}
	};
}

int main(int argc, char * argv[])
{
	return sdl_game::init<game_loop>(
	{
		.title = "Zombies",
		.width = 1280,
		.height = 768,
	});
}
