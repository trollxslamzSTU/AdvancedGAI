#include "sdl_game.h"
#include "slot_map.h"

#include <bitset>
#include <chrono>
#include <functional>
#include <vector>

namespace forester
{
	template<typename... Args> using goap_goal = std::function<bool(Args &...)>;

	/**
	 * An abstract action that may be tested against for completion, and performed if not, by a `goap_plan`.
	 */
	template<typename... Args> struct goap_action
	{
		virtual ~goap_action() = default;

		/**
		 * Performs the action on `args`.
		 */
		virtual void apply(Args &... args) = 0;

		/**
		 * Tests if the action is performanable based on the existing state of `args`.
		 */
		virtual bool test(Args const &... args) = 0;
	};

	/**
	 * The current step of a goal within a `goap_plan`.
	 */
	enum goap_step
	{
		goap_step_progress,
		goap_step_complete,
		goap_step_impossible,
	};

	/**
	 * A collection of `goap_actions` performable and a functor interface for using them to complete some kind of goal
	 * according to the external `Args` state.
	 */
	template<typename... Args> struct goap_plan final
	{
		goap_plan() = default;

		template<std::derived_from<goap_action<Args...>>... Actions> goap_plan(Actions &&... actions)
		{
			_available_actions.reserve(sizeof...(Actions));
			(_available_actions.push_back(std::make_unique<Actions>(std::forward<Actions>(actions))), ...);
		}

		/**
		 * Evaluates the next step in the plan according to the goal progress indicated by `is_goal_met` and the state
		 * in `args`, returning the next action to perform and a `goap_step` indicating if the plan is still in-
		 * progress, already completed, or impossible to perform.
		 *
		 * If `is_goal_met` is evaluated to `true` then the function returns with the `goap_step_complete` and no
		 * `goap_action` in the returned pairing.
		 *
		 * Similarly, no `goap_action` is returned if `goap_step_impossible` is returned in the pair.
		 *
		 * A pair is returned if the returned step is `goap_step_progress`.
		 */
		std::pair<goap_action<Args...> *, goap_step> operator()(goap_goal<Args...> const & is_goal_met, Args &... args) const
		{
			// HINT: This could be modified to make decisions based on cost of action as well as the flat listing order.
			if (is_goal_met && !is_goal_met(args...))
			{
				for (std::unique_ptr<goap_action<Args...>> const & action : _available_actions)
				{
					if (action->test(args...))
					{
						return {action.get(), goap_step_progress};
					}
				}

				return {nullptr, goap_step_impossible};
			}

			return {nullptr, goap_step_complete};
		}

		private:
		std::vector<std::unique_ptr<goap_action<Args...>>> _available_actions;
	};

	struct world_position;

	/**
	 * Represents a location in the world bound to the tile grid.
	 */
	struct world_coordinate final
	{
		/**
		 * Hashing functor for a `world_coordinate`.
		 */
		struct hash final
		{
			size_t operator()(world_coordinate const & coordinate) const;
		};

		world_coordinate() = default;

		explicit world_coordinate(world_position const & position);

		/**
		 * Returns the current chunk coordinate values.
		 */
		SDL_Point const & chunk() const { return _chunk; }

		/**
		 * Returns a new `world_coordinate` moved in the cardinal directions specified by `offset`.
		 */
		world_coordinate with_offset(SDL_Point const & offset) const;

		/**
		 * Returns the current X component of the position relative to the chunk coordinate.
		 */
		uint32_t const & x() const { return _xy.first; }

		/**
		 * Returns the current X and Y components of the position relative to the chunk coordinate.
		 */
		std::pair<uint32_t, uint32_t> const & xy() const { return _xy; }

		/**
		 * Returns the current Y component of the position relative to the chunk coordinate.
		 */
		uint32_t const & y() const { return _xy.second; }

		/**
		 * Tests if the coordinates are equivalent to `that`.
		 */
		bool operator==(world_coordinate const & that) const { return _chunk == that.chunk() && _xy == that.xy(); }

		private:
		SDL_Point _chunk = {0, 0};

		std::pair<uint32_t, uint32_t> _xy = {0, 0};
	};

	/**
	 * Represents a location in the world unbound from the tile grid.
	 */
	struct world_position final
	{
		world_position() {}

		explicit world_position(world_coordinate const & coordinate);

		explicit world_position(SDL_Point const & chunk, SDL_FPoint const & xy) : _chunk(chunk), _xy(xy) {}

		/**
		 * Returns the current chunk coordinate values.
		 */
		SDL_Point const & chunk() const { return _chunk; }

		/**
		 * Moves the position value by the absolute world position toward `direction` by the chunk distance of `step`.
		 */
		void move_direction(float const & step, SDL_Point const & direction);

		/**
		 * Moves the position value toward `position` by the chunk distance of `step`, returning whether or not it has
		 * arrived at the destination yet or not.
		 *
		 * This may be called sequentially over multiple frames to progressively move a position towards a given
		 * location.
		 */
		bool move_toward(float const & step, world_position const & position);

		/**
		 * Returns the current X component of the position relative to the chunk coordinate.
		 */
		float const & x() const { return _xy.x; }

		/**
		 * Returns the current X and Y components of the position relative to the chunk coordinate.
		 */
		SDL_FPoint const & xy() const { return _xy; }

		/**
		 * Returns the current Y component of the position relative to the chunk coordinate.
		 */
		float const & y() const { return _xy.y; }

		private:
		SDL_Point _chunk = {0, 0};

		SDL_FPoint _xy = {0, 0};
	};

	/**
	 * The kind of world tile that a given cell represents.
	 */
	enum world_tile
	{
		world_tile_water,
		world_tile_sand,
		world_tile_dirt,
		world_tile_grass,
		world_tile_stone,
		world_tile_snow,
		world_tile_max,
	};

	/**
	 * The kind of world object that a given cell holds.
	 */
	enum world_object
	{
		world_object_none,
		world_object_rock,
		world_object_tree,
		world_object_sapling,
		world_object_house,
		world_object_max,
	};

	/**
	 * A 16x16 grid of cells within the world.
	 */
	struct world_chunk final
	{
		/**
		 * A singular location unit within the grid.
		 */
		struct cell final
		{
			/**
			 * The current tile that represents the cell.
			 */
			world_tile tile = world_tile_water;

			/**
			 * The current object that the cell holds.
			 */
			world_object object = world_object_none;

			/**
			 * The health of the currently held cell object, where `1` is alive `0` and is dead.
			 */
			float object_health = 0;
		};

		/**
		 * The square-root of the cells per chunk.
		 */
		static constexpr size_t const cells_per_chunk_sqrt = 16;

		/**
		 * The number of cells in the chunk.
		 */
		static constexpr size_t const cells_per_chunk = cells_per_chunk_sqrt * cells_per_chunk_sqrt;

		/**
		 * Returns the cell from the chunk-relative `xy` coordinate.
		 *
		 * **Note** that coordinates outside of the 0 to `cells_per_chunk_sqrt - 1` range are erroneous.
		 */
		cell & operator[](std::pair<uint32_t, uint32_t> const & xy);

		/**
		 * Returns the cell from the chunk-relative `xy` coordinate.
		 *
		 * **Note** that coordinates outside of the 0 to `cells_per_chunk_sqrt - 1` range are erroneous.
		 */
		cell const & operator[](std::pair<uint32_t, uint32_t> const & xy) const;

		/**
		 * Returns the cell from the `index`.
		 *
		 * **Note** that indices outside of the 0 to `cells_per_chunk - 1` range are erroneous.
		 */
		cell & operator[](size_t const & index) { return _cells[index]; }

		/**
		 * Returns the cell from the `index`.
		 *
		 * **Note** that indices outside of the 0 to `cells_per_chunk - 1` range are erroneous.
		 */
		cell const & operator[](size_t const & index) const { return _cells[index]; }

		private:
		std::array<cell, cells_per_chunk_sqrt * cells_per_chunk_sqrt> _cells;
	};

	/**
	 * A generative collection of many `world_chunk`s and their information, as derived from a world seed.
	 */
	struct world_space final
	{
		/**
		 * A worker in the world.
		 */
		struct pawn final
		{
			/**
			 * State flags for a `pawn`.
			 */
			enum flag
			{
				flag_collected_wood,
				flag_collected_rock,
				flag_max = 32,
			};

			/**
			 * Global position in the world.
			 */
			world_position position;

			/**
			 * Active flags.
			 */
			std::bitset<flag_max> flags;
		};

		/**
		 * A handle to a `pawn` assigned to something, allowing them to be manipulated by some controller logic.
		 */
		using pawn_key = dod::slot_map_key32<pawn>;

		/**
		 * World timestamp.
		 */
		using timestamp = std::chrono::time_point<std::chrono::system_clock>;

		world_space() {}

		explicit world_space(uint32_t const & global_seed) : _global_seed(global_seed) {}

		/**
		 * Assigns a pawn, returning the `pawn_key` for it.
		 *
		 * Note that an invalid `pawn_key` may be returned if there are no currently available pawns. It should be
		 * checked for validity before use.
		 */
		pawn_key assign_pawn();

		/**
		 * Returns the `pawn`s currently assigned tasks as an iterable collection.
		 */
		dod::slot_map32<pawn> const & assigned_pawns() const { return _assigned_pawns; }

		/**
		 * Returns a `world_chunk` from the in-memory state or by generating it from the world seed.
		 */
		world_chunk fetch_chunk(SDL_Point const & coordinate) const;

		/**
		 * Checks if a chunk has changed from the initial conditions defined by the world seed.
		 *
		 * Returns the `world_chunk` and the timestamp of the last change if it has, otherwise `std::nullptr`.
		 */
		std::optional<std::pair<world_chunk const &, world_space::timestamp const &>> has_chunk_changed(SDL_Point const & coordinate) const;

		/**
		 * Returns a `world_chunk` generated from initial conditions of the world seed.
		 *
		 * **Note** that for getting the actual state of a chunk, `fetch_chunk` or `has_chunk_changed` are generally preferred.
		 */
		world_chunk generate_chunk(SDL_Point const & coordinate) const;

		/**
		 * Returns the `pawn` assigned to `key` or `nullptr` if the `pawn_key` is invalid.
		 *
		 * **Note** the returned pointer is only valid until `assign_pawn` or `unassign_pawn` is called.
		 */
		pawn * has_assigned_pawn(pawn_key const & key) { return _assigned_pawns.get(key); }

		/**
		 * Returns the `pawn` assigned to `key` or `nullptr` if the `pawn_key` is invalid.
		 *
		 * **Note** the returned pointer is only valid until `assign_pawn` or `unassign_pawn` is called.
		 */
		pawn const * has_assigned_pawn(pawn_key const & key) const { return _assigned_pawns.get(key); }

		/**
		 * Returns the local seed for a chunk at the given `coordinate`.
		 */
		uint32_t local_seed(SDL_Point const & coordinate) const;

		/**
		 * Queries the world from the absolute `coordinate` for `object`, returning the `world_coordinate` of one.
		 *
		 * **Note** that, as the world is functionally infinite, this will continue searching forever until a
		 * `world_object` matching `object` is found.
		 */
		world_coordinate query_object(world_coordinate const & coordinate, world_object const & object);

		/**
		 * Constructs a new `pawn` at `position`, adding it to the world.
		 */
		void spawn(world_position const & position);

		/**
		 * Unassigns the `pawn` referenced by `key` from the working state pool, returning whether or not `key` was valid.
		 *
		 * **Note** the provided `pawn_key` becomes invalid when the function returns.
		 */
		bool unassign_pawn(pawn_key const & key);

		/**
		 * Returns all `pawn`s not currently assigned to a task as an iterable collection.
		 */
		std::vector<pawn> const & unassigned_pawns() const { return _unassigned_pawns; }

		/**
		 * Writes `chunk` to the in-memory state for the chunk location of `coordinate`.
		 */
		void write_chunk(SDL_Point const & coordinate, world_chunk const & chunk);

		private:
		using saved_chunk_map = std::unordered_map<SDL_Point, std::pair<world_chunk, timestamp>>;

		saved_chunk_map _saved_chunks;

		uint32_t _global_seed = 0;

		dod::slot_map32<pawn> _assigned_pawns;

		std::vector<pawn> _unassigned_pawns;
	};
}
