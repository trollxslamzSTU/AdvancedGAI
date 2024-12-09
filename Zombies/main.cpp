#include "zombies.h"

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
		zombie_archetype_hunter,
		zombie_archetype_spotter,
	};

	struct zombie_actor final
	{
		entity_location location;

		float health = 1.f;

		zombie_archetype archetype = zombie_archetype_hunter;

		bool is_alerted = false;

		sdl_game::countdown damage_flash;
	};

	SDL_FPoint zombie_sightline(zombie_actor const & zombie)
	{
		float range = 0;

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
		}

		return sdl_game::projected_point(zombie.location.position(), zombie.location.orientation(), range);
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
				.location = entity_location(0, {1200, 500}),
				.archetype = zombie_archetype_hunter,
			});

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {1200, 700}),
				.archetype = zombie_archetype_spotter,
			});

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {450, 500}),
				.archetype = zombie_archetype_spotter,
			});

			_zombies.emplace(zombie_actor
			{
				.location = entity_location(0, {225, 470}),
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

			constexpr float const player_speed = 0.5f;
			SDL_FPoint const movement = to_fpoint(key_direction) * to_fpoint(player_speed);
			SDL_FPoint moved_player_position = _player.position() + movement;
			auto const player_area = circle_shape(moved_player_position, 24.f);

			for (auto const [coordinate, _] : _level.walls())
			{
				SDL_FRect const wall_rect =
				{
					.x = static_cast<float>(coordinate.x * tile_size.x),
					.y = static_cast<float>(coordinate.y * tile_size.y),
					.w = tile_size.x,
					.h = tile_size.y,
				};

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
				// TODO: Implement your zombie "squad" behavior.
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
			context.render_line(tint, zombie.location.position(), zombie_sightline(zombie));
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
