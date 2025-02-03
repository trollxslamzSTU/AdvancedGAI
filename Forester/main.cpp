#include "forester.h"
#include "slot_map.h"

#include <bitset>
#include <format>
#include <deque>

using namespace forester;

namespace
{
	struct walking_work final
	{
		world_coordinate target;

		world_coordinate origin;

		world_space::pawn_key pawn_key;

		bool at_target = false;

		static bool is_complete(world_space& space, walking_work& work)
		{
			return work.at_target;
		}
	};

	struct building_work final
	{
		world_coordinate target;

		world_coordinate origin;

		world_space::pawn_key pawn_key;

		bool at_target = false;
		bool at_target_resource = false;
		bool found_target_resource = false;

		world_coordinate has_tree;
		world_coordinate has_rock;

		bool rock_gotten = false;
		bool wood_gotten = false;

		bool house_built = false;

		static bool is_complete(world_space& space, building_work& work)
		{
			return work.house_built;
		}
	};

	std::optional<world_space::pawn::flag> object_collection_flag(world_object const& object)
	{
		using enum world_space::pawn::flag;

		switch (object)
		{
		case world_object_tree: return flag_collected_wood;
		case world_object_rock: return flag_collected_rock;
		default: return std::nullopt;
		}
	}

	struct assign_pawn final : public goap_action<world_space, walking_work>
	{
		bool test(world_space const& space, walking_work const& work) override
		{
			return !work.pawn_key && !space.unassigned_pawns().empty();
		}

		void apply(world_space& space, walking_work& work) override
		{
			SDL_assert(!work.pawn_key);

			work.pawn_key = space.assign_pawn();

			if (world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key))
			{
				work.origin = world_coordinate(space.has_assigned_pawn(work.pawn_key)->position);
			}
		}
	};

	struct goto_target final : public goap_action<world_space, walking_work>
	{
		bool test(world_space const& space, walking_work const& work) override
		{
			return work.pawn_key && !work.at_target;
		}

		void apply(world_space& space, walking_work& work) override
		{
			world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key);

			if (!pawn->position.move_toward(0.001f, world_position(work.target)))
			{
				work.at_target = true;

				space.unassign_pawn(work.pawn_key);
			}
		}
	};

	struct house_assign_pawn final : public goap_action<world_space, building_work>
	{
		bool test(world_space const& space, building_work const& work) override
		{
			
			return !work.pawn_key && !space.unassigned_pawns().empty();
		}

		void apply(world_space& space, building_work& work) override
		{
			SDL_assert(!work.pawn_key);
			
			work.pawn_key = space.assign_pawn();

			if (world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key))
			{
				work.origin = world_coordinate(space.has_assigned_pawn(work.pawn_key)->position);
			}
		}
	};

	struct house_goto_target_resource final : public goap_action<world_space, building_work>
	{
		bool test(world_space const& space, building_work const& work) override
		{
			return work.pawn_key && !work.at_target_resource;
		}

		void apply(world_space& space, building_work& work) override
		{
			world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key);
			
			if (work.rock_gotten)
			{
				if (!pawn->position.move_toward(0.001f, world_position(work.has_tree)))
				{
					work.at_target_resource = true;

					//space.unassign_pawn(work.pawn_key);
				}
			}
			if (work.wood_gotten)
			{
				if (!pawn->position.move_toward(0.001f, world_position(work.has_rock)))
				{
					work.at_target_resource = true;

					//space.unassign_pawn(work.pawn_key);
				}
			}
			if ((!work.wood_gotten && !work.rock_gotten))
			{
				if (!pawn->position.move_toward(0.001f, world_position(work.has_tree)))
				{
					work.at_target_resource = true;

					//space.unassign_pawn(work.pawn_key);
				}
			}
		}
	};

	struct house_goto_target final : public goap_action<world_space, building_work>
	{
		bool test(world_space const& space, building_work const& work) override
		{
			return work.pawn_key && !work.at_target;
		}

		void apply(world_space& space, building_work& work) override
		{
			world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key);

			if (!pawn->position.move_toward(0.001f, world_position(work.target)))
			{
				work.at_target = true;

				//space.unassign_pawn(work.pawn_key);
			}
		}
	};

	struct house_building final : public goap_action<world_space, building_work>
	{
		bool test(world_space const& space, building_work const& work) override
		{
			return !work.pawn_key && !space.unassigned_pawns().empty();
		}

		void apply(world_space& space, building_work& work) override
		{
			work.house_built = true;
			space.unassign_pawn(work.pawn_key);
		}
	};

	struct find_object final : public goap_action<world_space, building_work>
	{
		bool test(world_space const& space, building_work const& work) override
		{
			const world_space::pawn* pawn = space.has_assigned_pawn(work.pawn_key);
			if (!pawn->flags.test(world_space::pawn::flag_collected_wood))
			{
				return true;
			}
			if (pawn->flags.test(world_space::pawn::flag_collected_rock))
			{
				return true;
			}
			return work.pawn_key && !work.found_target_resource;
		}

		void apply(world_space& space, building_work& work) override
		{
			world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key);
			if (pawn->flags.test(world_space::pawn::flag_collected_wood))
			{
				work.wood_gotten = true;
				return;
			};
			if (pawn->flags.test(world_space::pawn::flag_collected_rock))
			{
				work.rock_gotten = true;
				return;
			};
			
			work.has_rock = space.query_object(work.target, world_object_rock);
			work.has_tree = space.query_object(work.target, world_object_tree);
		

			work.found_target_resource = true;
		}
	};

	struct destroy_object final : public goap_action<world_space, building_work>
	{
		bool test(world_space const& space, building_work const& work) override
		{
			SDL_Point chunk = work.has_rock.chunk();
			world_chunk copy = space.fetch_chunk(chunk);
			return !work.pawn_key && !space.unassigned_pawns().empty() && copy[work.has_tree.xy()].object == world_object_none;
		}

		void apply(world_space& space, building_work& work) override
		{
			world_space::pawn* const pawn = space.has_assigned_pawn(work.pawn_key);
			SDL_Point chunk = work.has_rock.chunk();
			world_chunk copy = space.fetch_chunk(chunk);

			if (copy[work.has_tree.xy()].object == world_object_tree)
			{
				copy[work.has_tree.xy()].object = world_object_none;
				pawn->flags.set(world_space::pawn::flag_collected_wood);
			}
			
			if (copy[work.has_rock.xy()].object == world_object_rock)
			{
				copy[work.has_rock.xy()].object = world_object_none;
				pawn->flags.set(world_space::pawn::flag_collected_rock);
			}
			space.write_chunk(chunk, copy);
		}
	};

	struct game_loop final
	{
		bool on_ready(sdl_game::app_context& context)
		{
			using sdl_game::to_fpoint;

			constexpr auto const default_texture_filtering = sdl_game::texture_filtering_linear;

			if (!(_entities_texture = context.load_texture("assets/entities.png", default_texture_filtering)))
			{
				return false;
			}

			if (!(_ground_tiles_texture = context.load_texture("assets/ground_tiles.png", sdl_game::texture_filtering_nearest)))
			{
				return false;
			}

			if (!(_structures_texture = context.load_texture("assets/structures.png", default_texture_filtering)))
			{
				return false;
			}

			for (size_t i = 0; i < _rock_textures.size(); i += 1)
			{
				if (!(_rock_textures[i] = context.load_texture(std::format("assets/rock_{:02}.png", i + 1), default_texture_filtering)))
				{
					return false;
				}
			}

			for (size_t i = 0; i < _tree_textures.size(); i += 1)
			{
				if (!(_tree_textures[i] = context.load_texture(std::format("assets/tree_{:02}.png", i + 1), default_texture_filtering)))
				{
					return false;
				}
			}

			_space = world_space(297277263);
			_cell_size = to_fpoint(context.query_texture(_ground_tiles_texture).height);
			_entity_size = to_fpoint(context.query_texture(_entities_texture).height);
			_chunk_size = to_fpoint(world_chunk::cells_per_chunk_sqrt) * _cell_size;

			for (size_t i = 0; i < _fetched_chunks.size(); i += 1)
			{
				_fetched_chunks[i] = _space.fetch_chunk(chunk_offsets[i]);
			}

			for (size_t i = 0; i < 5; i += 1)
			{
				_space.spawn(world_position({ 0, 0 }, { 0.025f * i, 0 }));
			}

			return true;
		}

		void on_update(sdl_game::app_context const& context)
		{
			SDL_Point const key_direction = context.key_axis(
				{
					.x_neg = SDL_SCANCODE_A,
					.x_pos = SDL_SCANCODE_D,
					.y_neg = SDL_SCANCODE_W,
					.y_pos = SDL_SCANCODE_S,
				});

			for (size_t i = 0; i < _fetched_chunks.size(); i += 1)
			{
				std::optional saved_chunk = _space.has_chunk_changed(_camera_position.chunk() + chunk_offsets[i]);

				if (!saved_chunk)
				{
					continue;
				}

				auto const& [chunk, timestamp] = *saved_chunk;

				if (_fetched_timestamps[i] < timestamp)
				{
					_fetched_chunks[i] = chunk;
				}
			}

			if (key_direction.x || key_direction.y)
			{
				constexpr float const camera_speed = 0.002f;

				_camera_position.move_direction(camera_speed,
					{
						.x = key_direction.x,
						.y = key_direction.y,
					});

				if (_camera_position.chunk() != _last_fetched_chunk)
				{
					for (size_t i = 0; i < _fetched_chunks.size(); i += 1)
					{
						_fetched_chunks[i] = _space.fetch_chunk(_camera_position.chunk() + chunk_offsets[i]);
					}
				}

				_last_fetched_chunk = _camera_position.chunk();
			}

			if (context.mouse_buttons().is_pressed(SDL_BUTTON_LEFT))
			{
				_walking_work.push_back(walking_work
					{
						.target = world_coordinate(screen_to_world(context, context.mouse_position())),
					});
			}

			if (context.mouse_buttons().is_pressed(SDL_BUTTON_RIGHT))
			{
				
				_building_work.push_back(building_work
					{
						.target = world_coordinate(screen_to_world(context, context.mouse_position())),
					});
			}

			if (context.key_buttons().is_pressed(SDL_SCANCODE_H))
			{
				_is_showing_grid = !_is_showing_grid;
			}

			if (context.key_buttons().is_pressed(SDL_SCANCODE_Q))
			{
				_walking_work.push_back(walking_work
					{
						.target = world_coordinate(),
					});
			}

			if (!_walking_work.empty())
			{
				walking_work work = _walking_work.front();

				_walking_work.pop_front();

				auto const [action, step] = _walking_plan(walking_work::is_complete, _space, work);

				switch (step)
				{
				default: sdl_game::unreachable();
				case goap_step_complete: break;

				case goap_step_impossible:
				{
					_walking_work.push_back(work);
				}
				break;

				case goap_step_progress:
				{
					action->apply(_space, work);
					_walking_work.push_back(work);
				}
				break;
				}
			}

			if (!_building_work.empty())
			{
				building_work work = _building_work.front();

				_building_work.pop_front();

				auto const [action, step] = _building_plan(building_work::is_complete, _space, work);

				switch (step)
				{
				default: sdl_game::unreachable();
				case goap_step_complete: break;

				case goap_step_impossible:
				{
					_building_work.push_back(work);
				}
				break;

				case goap_step_progress:
				{
					action->apply(_space, work);
					_building_work.push_back(work);
				}
				break;
				}
			}
		}

		void on_render(sdl_game::app_context& context)
		{
			using sdl_game::to_fpoint;

			sdl_game::texture_info const renderer_info = context.query_renderer();

			SDL_FPoint const render_center =
			{
				.x = (renderer_info.width / 2.f) - (_chunk_size.x * _camera_position.x()),
				.y = (renderer_info.height / 2.f) - (_chunk_size.y * _camera_position.y()),
			};

			for (size_t i = 0; i < _fetched_chunks.size(); i += 1)
			{
				render_chunk(context, _fetched_chunks[i], render_center + (_chunk_size * to_fpoint(chunk_offsets[i])));
			}

			SDL_FPoint const actor_texture_center = _entity_size / to_fpoint(2.f);

			for (world_space::pawn const& pawn : _space.unassigned_pawns())
			{
				SDL_FPoint const screen_position = world_to_screen(context, pawn.position);

				context.render_sprite(_entities_texture, screen_position - actor_texture_center, {});
			}

			for (world_space::pawn const& pawn : _space.assigned_pawns())
			{
				SDL_FPoint const screen_position = world_to_screen(context, pawn.position);

				context.render_sprite(_entities_texture, screen_position - actor_texture_center,
					{
						.tint_color = sdl_game::rgb(0.f, 1.f, 0.f),
					});
			}

			for (walking_work const& work : _walking_work)
			{
				SDL_FPoint const screen_position = world_to_screen(context, world_position(work.target)) - (_cell_size / to_fpoint(2.f));

				context.render_rect_outline(sdl_game::rgb(0.f, 1.f, 0.f), 2.f,
					{
						.x = screen_position.x,
						.y = screen_position.y,
						.w = _cell_size.x,
						.h = _cell_size.y,
					});
			}

			for (building_work const& work : _building_work)
			{
				SDL_FPoint const screen_position = world_to_screen(context, world_position(work.target)) - (_cell_size / to_fpoint(2.f));

				context.render_rect_outline(sdl_game::rgb(0.f, 1.f, 0.f), 2.f,
					{
						.x = screen_position.x,
						.y = screen_position.y,
						.w = _cell_size.x,
						.h = _cell_size.y,
					});
			}
		}

	private:
		static constexpr std::array<SDL_Point, 9> const chunk_offsets =
		{
			SDL_Point(-1, -1),
			SDL_Point(0, -1),
			SDL_Point(1, -1),
			SDL_Point(-1, 0),
			SDL_Point(0, 0),
			SDL_Point(1, 0),
			SDL_Point(-1, 1),
			SDL_Point(0, 1),
			SDL_Point(1, 1),
		};

		world_space _space;

		std::shared_ptr<SDL_Texture> _entities_texture;

		world_position _camera_position = world_position({ 0, 0 }, { 0.5f, 0.5f });

		SDL_Point _last_fetched_chunk = { 0, 0 };

		SDL_FPoint _cell_size = {};

		SDL_FPoint _chunk_size = {};

		SDL_FPoint _entity_size = {};

		std::array<world_chunk, 9> _fetched_chunks;

		std::array<world_space::timestamp, 9> _fetched_timestamps;

		std::shared_ptr<SDL_Texture> _ground_tiles_texture;

		std::array<std::shared_ptr<SDL_Texture>, 2> _rock_textures;

		std::array<std::shared_ptr<SDL_Texture>, 2> _tree_textures;

		std::shared_ptr<SDL_Texture> _structures_texture;

		std::deque<walking_work> _walking_work;

		std::deque<building_work> _building_work;

		goap_plan<world_space, walking_work> _walking_plan =
		{
			assign_pawn(),
			goto_target(),
		};

		goap_plan<world_space, building_work> _building_plan =
		{
			house_assign_pawn(),
			find_object(),
			house_goto_target_resource(),
			destroy_object(),
			house_goto_target_resource(),
			destroy_object(),
			//house_goto_target(),
			house_building(),
		};

		bool _is_showing_grid = false;

		void render_chunk(sdl_game::app_context& context, world_chunk const& chunk, SDL_FPoint const& position)
		{
			for (size_t i = 0; i < world_chunk::cells_per_chunk; i += 1)
			{
				size_t const cell_y = i / world_chunk::cells_per_chunk_sqrt;
				size_t const cell_x = i % world_chunk::cells_per_chunk_sqrt;

				SDL_FPoint const cell_position =
				{
					.x = position.x + static_cast<float>(cell_x * _cell_size.x),
					.y = position.y + static_cast<float>(cell_y * _cell_size.y),
				};

				world_chunk::cell const& cell = chunk[i];

				context.render_sheet(_ground_tiles_texture, cell_position,
					{
						.columns = world_tile_max,
						.frame = static_cast<uint16_t>(cell.tile),
					});

				switch (cell.object)
				{
				case world_object_rock:
				{
					if (cell.object_health > 0)
					{
						auto const rock_variant = [&cell]
							{
								switch (cell.tile)
								{
								case world_tile_stone:
								case world_tile_sand: return 1;
								default: return 0;
								}
							};

						context.render_sheet(_rock_textures[rock_variant()], cell_position,
							{
								.columns = 6,
								.frame = static_cast<size_t>(std::floor(6.f * cell.object_health)),
							});
					}
				};
				break;

				case world_object_tree:
				{
					if (cell.object_health > 0)
					{
						auto const tree_variant = [&cell]
							{
								switch (cell.tile)
								{
								case world_tile_stone:
								case world_tile_snow: return 1;
								default: return 0;
								}
							};

						context.render_sheet(_tree_textures[tree_variant()], cell_position,
							{
								.columns = 4,
								.frame = static_cast<size_t>(std::floor(3.f * cell.object_health)),
							});
					}
				};
				break;

				case world_object_house:
				{
					context.render_sheet(_structures_texture, cell_position, {});
				};
				break;

				case world_object_none: break;
				default: sdl_game::unreachable();
				}

				if (_is_showing_grid)
				{
					context.render_rect_outline(sdl_game::rgb(0.f, 0.f, 1.f), 1.f,
						{
							.x = cell_position.x,
							.y = cell_position.y,
							.w = _cell_size.x,
							.h = _cell_size.y,
						});
				}
			}

			if (_is_showing_grid)
			{
				context.render_rect_outline(sdl_game::rgb(1.f, 0.f, 0.f), 1.f,
					{
						.x = position.x,
						.y = position.y,
						.w = _chunk_size.x,
						.h = _chunk_size.y,
					});
			}
		}

		world_position screen_to_world(sdl_game::app_context const& context, SDL_FPoint const& screen_position) const
		{
			using sdl_game::to_fpoint;

			sdl_game::texture_info const renderer_info = context.query_renderer();
			SDL_FPoint const chunk_size = to_fpoint(world_chunk::cells_per_chunk_sqrt) * _cell_size;
			SDL_FPoint const screen_midpoint = { renderer_info.width / 2.f, renderer_info.height / 2.f };
			SDL_FPoint const chunk_position = screen_position - (screen_midpoint - (chunk_size * _camera_position.xy()));
			SDL_FPoint const absolute_position = (chunk_position + (chunk_size * to_fpoint(_camera_position.chunk()))) / chunk_size;
			SDL_Point const chunk = sdl_game::to_point({ std::floor(absolute_position.x), std::floor(absolute_position.y) });

			return world_position(chunk, absolute_position - to_fpoint(chunk));
		}

		SDL_FPoint world_to_screen(sdl_game::app_context const& context, world_position const& position) const
		{
			SDL_Point const offset_coord = (position.chunk() - _camera_position.chunk());
			sdl_game::texture_info const render_info = context.query_renderer();
			SDL_FPoint const chunk_size = sdl_game::to_fpoint(world_chunk::cells_per_chunk_sqrt) * _cell_size;

			return
			{
				.x = (chunk_size.x * offset_coord.x) + (render_info.width / 2.f) + (chunk_size.x * (position.xy().x - _camera_position.xy().x)),
				.y = (chunk_size.y * offset_coord.y) + (render_info.height / 2.f) + (chunk_size.y * (position.xy().y - _camera_position.xy().y)),
			};
		}
	};
}

int main(int argc, char* argv[])
{
	return sdl_game::init<game_loop>(
		{
			.title = "Forester",
			.width = 1280,
			.height = 720,
		});
}