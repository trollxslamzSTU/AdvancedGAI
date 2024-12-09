#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <chrono>
#include <utility>
#include <memory>
#include <random>
#include <compare>
#include <array>
#include <optional>
#include <cmath>
#include <bitset>

#ifdef _WIN32
#include <Windows.h>
#endif

#pragma warning(disable: 26819)
#include "SDL.h"
#pragma warning(default: 26819)

#include "SDL_image.h"
#include "SDL_ttf.h"

namespace std
{
	template<> struct hash<SDL_Point>
	{
		size_t operator()(SDL_Point const & value) const
		{
			size_t const hash_x = std::hash<int>()(value.x);
			size_t const hash_y = std::hash<int>()(value.y);

			return hash_x ^ (hash_y << 1);
		}
	};
}

namespace sdl_game
{
	template<typename T, typename... U> concept any_of = std::disjunction_v<std::is_same<T, U>...>;

	template<typename T> concept point_type = any_of<T, SDL_Point, SDL_FPoint>;
}

template<sdl_game::point_type A, sdl_game::point_type B> inline bool operator==(A const & a, B const & b)
{
	return a.x == b.x && a.y == b.y;
}

template<sdl_game::point_type P> inline P operator+(P const & a, P const & b)
{
	return {a.x + b.x, a.y + b.y};
}

template<sdl_game::point_type P> inline P operator-(P const & a, P const & b)
{
	return {a.x - b.x, a.y - b.y};
}

template<sdl_game::point_type P> inline P operator*(P const & a, P const & b)
{
	return {a.x * b.x, a.y * b.y};
}

template<sdl_game::point_type P> inline P operator/(P const & a, P const & b)
{
	return {a.x / b.x, a.y / b.y};
}

template<sdl_game::point_type P> inline P operator%(P const & a, P const & b)
{
	return {a.x % b.x, a.y % b.y};
}

template<sdl_game::point_type P> inline SDL_FPoint operator-(P const & a)
{
	return {-a.x, -a.y};
}

namespace sdl_game
{
	constexpr inline SDL_FPoint to_fpoint(float const & scalar)
	{
		return
		{
			.x = scalar,
			.y = scalar,
		};
	}

	constexpr inline SDL_FPoint to_fpoint(SDL_Point const & point)
	{
		return
		{
			.x = static_cast<float>(point.x),
			.y = static_cast<float>(point.y),
		};
	}

	constexpr inline SDL_Point to_point(SDL_FPoint const& point)
	{
		return
		{
			.x = static_cast<int>(point.x),
			.y = static_cast<int>(point.y),
		};
	}

	constexpr inline SDL_Point to_point(int const & scalar)
	{
		return
		{
			.x = scalar,
			.y = scalar,
		};
	}

	struct sdl_deleter final
	{
		void operator()(SDL_Window* window) const
		{
			SDL_DestroyWindow(window);
		}

		void operator()(SDL_Renderer* renderer) const
		{
			SDL_DestroyRenderer(renderer);
		}

		void operator()(SDL_Texture* texture) const
		{
			SDL_DestroyTexture(texture);
		}

		void operator()(SDL_Surface* surface) const
		{
			SDL_FreeSurface(surface);
		}

		void operator()(TTF_Font* font) const
		{
			TTF_CloseFont(font);
		}
	};

	inline constexpr SDL_Color greyscale(float intensity, float alpha_intensity = 1.f)
	{
		auto const value = static_cast<uint8_t>(UINT8_MAX * intensity);
		auto const alpha_value = static_cast<uint8_t>(UINT8_MAX * alpha_intensity);

		return {value, value, value, alpha_value};
	}

	inline constexpr SDL_Color rgb(float r_intensity, float g_intensity, float b_intensity)
	{
		auto const r_value = static_cast<uint8_t>(UINT8_MAX * r_intensity);
		auto const g_value = static_cast<uint8_t>(UINT8_MAX * g_intensity);
		auto const b_value = static_cast<uint8_t>(UINT8_MAX * b_intensity);

		return {r_value, g_value, b_value, UINT8_MAX};
	}

	struct app_info final
	{
		char const * title = nullptr;

		uint16_t width = 1280;

		uint16_t height = 720;

		float target_frame_time = 1.f / 60.f;
	};

	struct sprite_font final
	{
		static constexpr char glyph_head = 32;

		static constexpr char glyph_tail = 126;

		struct glyph_metrics final
		{
			SDL_Rect region = {};

			float advance = 0;
		};

		float line_height = 0;

		std::shared_ptr<SDL_Texture> atlas;

		std::array<glyph_metrics, (glyph_tail - glyph_head) + 1> glyphs;

		explicit operator bool()
		{
			return atlas != nullptr;
		}
	};

	struct text_metrics final
	{
		float width = 0;

		float height = 0;

		size_t line_count = 0;

		static text_metrics measure_text(sprite_font const & font, std::string_view const & text);
	};

	struct texture_info
	{
		uint16_t width = 0, height = 0;
	};

	template<typename E, E max> struct button_set final
	{
		void clear_frame()
		{
			_pressed.reset();
			_released.reset();
		}

		bool is_held(E const & element) const
		{
			return _held[element];
		}

		bool is_pressed(E const & element) const
		{
			return _pressed[element];
		}

		bool is_released(E const & element) const
		{
			return _released[element];
		}

		void set(E const & element, bool const & state)
		{
			_held.set(element, state);

			if (state)
			{
				_pressed.set(element, true);
			}
			else
			{
				_released.set(element, true);
			}
		}

		private:
		using bitset = std::bitset<max>;

		bitset _held = {};

		bitset _pressed = {};

		bitset _released = {};
	};

	template<point_type P> inline float point_distance_squared(P const & a, P const & b)
	{
		P const to =
		{
			.x = b.x - a.x,
			.y = b.y - a.y,
		};

		return (to.x * to.x) + (to.y * to.y);
	}

	template<point_type P> inline float point_distance(P const & a, P const & b)
	{
		return std::sqrt(point_distance_squared(a, b));
	}

	inline SDL_FPoint projected_point(SDL_FPoint const & origin, float const & angle, float const & distance)
	{
		float const angle_radians = angle * (static_cast<float>(M_PI) / 180.f);

		return
		{
			.x = origin.x + (std::cos(angle_radians) * distance),
   			.y = origin.y + (std::sin(angle_radians) * distance),
		};
	}

	inline float angle_difference(float const & from, float const & to)
	{
		constexpr float const tau = static_cast<float>(M_PI * 2 * (180.f / M_PI));
		float const difference = std::fmod(to - from, tau);

		return std::fmod(2.f * difference, tau) - difference;
	}

	inline float lerped_angle(float const & from, float const & to, float const & weight)
	{
		return std::fmod(from + (angle_difference(from, to) * weight), 360.f);
	}

	struct circle_shape final
	{
		explicit circle_shape(SDL_FPoint const & origin, float const & radius) : _origin(origin), _radius(radius) {}

		std::optional<SDL_FPoint> has_line_intersection(SDL_FPoint const & p1, SDL_FPoint const & p2) const;

		std::optional<SDL_FPoint> has_rect_intersection(SDL_FRect const & rect) const;

		bool is_circle_intersecting(SDL_FPoint const & origin, float const & radius) const;

		SDL_FPoint const & origin() const { return _origin; }

		float const & radius() const { return _radius; }

		private:
		SDL_FPoint _origin = {};

		float _radius = 0;
	};

	enum texture_filtering
	{
		texture_filtering_nearest,
		texture_filtering_linear,
		texture_filtering_max,
	};

	struct app_context final
	{
		using key_button_set = button_set<SDL_Scancode, SDL_NUM_SCANCODES>;

		using mouse_button_set = button_set<uint32_t, 8>;

		struct key_axis_mapping final
		{
			SDL_Scancode x_neg = SDL_SCANCODE_UNKNOWN, x_pos = SDL_SCANCODE_UNKNOWN;

			SDL_Scancode y_neg = SDL_SCANCODE_UNKNOWN, y_pos = SDL_SCANCODE_UNKNOWN;
		};

		struct render_atlas_options final
		{
			float rotation = 0;

			SDL_Color tint_color = greyscale(1);

			SDL_Rect region = {};

			std::optional<SDL_FPoint> center;
		};

		struct render_font_options final
		{
			SDL_Color fill_color = greyscale(1);
		};

		struct render_sheet_options final
		{
			float rotation = 0;

			uint16_t rows = 1;

			uint16_t columns = 1;

			size_t frame = 0;

			SDL_Color tint_color = greyscale(1);

			std::optional<SDL_FPoint> center;
		};

		struct render_sprite_options final
		{
			float rotation = 0;

			SDL_Color tint_color = greyscale(1);
		};

		explicit app_context(SDL_Renderer * renderer) : _renderer(renderer) {}

		void clear_frame();

		void consume_event(SDL_Event const & event);

		bool has_quit() const { return _has_quit; }

		std::shared_ptr<SDL_Texture> load_texture(std::string const & image_path, texture_filtering const & filtering);

		sprite_font load_sprite_font(std::string const & font_path, uint8_t const & font_size);

		texture_info query_renderer() const;

		texture_info query_texture(std::shared_ptr<SDL_Texture> const & texture) const;

		void quit();

		SDL_Point key_axis(key_axis_mapping const & mapping) const;

		key_button_set const & key_buttons() const { return _key_buttons; }

		mouse_button_set const & mouse_buttons() const { return _mouse_buttons; }

		SDL_FPoint const & mouse_position() const { return _mouse_position; }

		std::mt19937 & randomness() { return _randomness; }

		void render_atlas(std::shared_ptr<SDL_Texture> const & atlas, SDL_FPoint const & origin, render_atlas_options const & options);

		void render_clear(SDL_Color const & clear_color = greyscale(0));

		void render_point(SDL_Color const & point_color, SDL_FPoint const & point);

		void render_rect_outline(SDL_Color const & color, float const & width, SDL_FRect const & rect);

		void render_rect_solid(SDL_Color const & color, SDL_FRect const & rect);

		void render_sheet(std::shared_ptr<SDL_Texture> const & sheet, SDL_FPoint origin, render_sheet_options const & options);

		void render_sprite(std::shared_ptr<SDL_Texture> const & sprite, SDL_FPoint origin, render_sprite_options const & options);

		void render_font(sprite_font const & font, SDL_FPoint const & origin, std::string_view const & text, render_font_options const & options);

		void render_line(SDL_Color const & color, SDL_FPoint const & origin, SDL_FPoint const & target);

		SDL_Renderer * renderer() const
		{
			return _renderer;
		}

		void set_render_clip(std::optional<SDL_Rect> const & clip_rect);

		private:
		SDL_Renderer * _renderer = nullptr;

		std::mt19937 _randomness = std::mt19937(std::random_device()());

		bool _has_quit = false;

		key_button_set _key_buttons = {};

		mouse_button_set _mouse_buttons = {};

		SDL_FPoint _mouse_position = {};
	};

	struct countdown final
	{
		using duration = std::chrono::duration<float>;

		countdown() : countdown(0) {}

		explicit countdown(float const & duration_seconds) : _remaining_seconds(duration_seconds) {}

		void reset(float const & duration_seconds);

		bool tick();

		duration remaining() const;

		bool is_zero() const;

		private:
		float _remaining_seconds = 0;
	};

	template<typename State>
	concept game_stateable = requires (State state, app_context context)
	{
		{ state.on_ready(context) } -> std::convertible_to<bool>;
		{ state.on_render(context) } -> std::same_as<void>;
	};

#if defined(__clang__) || defined(__GNUC__)
	[[noreturn]] inline void unreachable()
	{
		__builtin_unreachable();
	}
#elif defined(_MSC_VER)
	[[noreturn]] inline void unreachable()
	{
		__assume(0);
	}
#else
	[[noreturn]] inline void unreachable()
	{
		std::abort();
	}
#endif

	template<game_stateable GameState>
	int init(app_info const & info)
	{
		struct sdl_subsystem_states final
		{
			explicit sdl_subsystem_states() {}

			~sdl_subsystem_states()
			{
				if (TTF_WasInit())
				{
					TTF_Quit();
				}

				if (_was_img_init)
				{
					IMG_Quit();
				}

				SDL_Quit();
			}

			bool init(uint32_t const init_flags)
			{
				return SDL_Init(init_flags) == 0;
			}

			bool init_ttf()
			{
				return TTF_Init() == 0;
			}

			bool init_img()
			{
				_was_img_init = IMG_Init(IMG_INIT_PNG) == 0;

				return _was_img_init;
			}

			private:
			bool _was_img_init = false;
		};

		sdl_subsystem_states sdl_subsystems;

		if (!sdl_subsystems.init(SDL_INIT_EVERYTHING))
		{
			return EXIT_FAILURE;
		}

		if (!sdl_subsystems.init_ttf())
		{
			return EXIT_FAILURE;
		}

#if defined(_WIN32) && defined(_DEBUG)
		AllocConsole();

		_CrtSetAllocHook([](int alloc_type, void * user_data, size_t size, int block_type, long request_number, uint8_t const * file_name, int line_number)
		{
			if (block_type != _CRT_BLOCK && size)
			{
				SDL_Log("%zu bytes allocated\n", size);
			}

			return TRUE;
		});
#endif

		constexpr int const window_pos = SDL_WINDOWPOS_CENTERED;
		constexpr uint32_t const window_flags = SDL_WINDOW_SHOWN;

		auto const window = std::unique_ptr<SDL_Window, sdl_deleter>(SDL_CreateWindow(
			info.title,
			window_pos,
			window_pos,
			info.width,
			info.height,
			window_flags));

		if (!window)
		{
			return EXIT_FAILURE;
		}

		constexpr uint32_t const renderer_flags = 0;
		auto const renderer = std::unique_ptr<SDL_Renderer, sdl_deleter>{SDL_CreateRenderer(window.get(), -1, renderer_flags)};

		if (!renderer)
		{
			return EXIT_FAILURE;
		}

		auto context = app_context(renderer.get());

		auto const run_loop = [&](GameState & state)
		{
			if (!state.on_ready(context))
			{
				return EXIT_FAILURE;
			}

			uint64_t const ticks_initial = SDL_GetTicks64();
			uint64_t ticks_previous = ticks_initial;
			float accumulated_time = 0;

			while (true)
			{
				SDL_Event event;

				while (SDL_PollEvent(&event) != 0)
				{
					context.consume_event(event);

					if constexpr (requires (GameState state, app_context & context, SDL_Event const & event) { { state.on_event(context, event) } -> std::same_as<void>; })
					{
						state.on_event(context, event);
					}

					if (context.has_quit())
					{
						if constexpr (requires (GameState state) { { state.on_finish() } -> std::same_as<void>; })
						{
							state.on_finish();
						}

						return EXIT_SUCCESS;
					}
				}

				constexpr float const milliseconds_per_second = 1000.0;
				uint64_t const ticks_current = SDL_GetTicks64();
				float const actual_frame_time = static_cast<float>(ticks_current - ticks_previous) / milliseconds_per_second;
				auto const delta_time = std::max(info.target_frame_time, actual_frame_time);

				ticks_previous = ticks_current;
				accumulated_time += delta_time;

				if constexpr (requires (GameState state, app_context const & context) { { state.on_update(context) } -> std::same_as<void>; })
				{
					while (accumulated_time >= info.target_frame_time)
					{
						state.on_update(context);

						accumulated_time -= info.target_frame_time;
					}
				}

				state.on_render(context);
				SDL_RenderPresent(renderer.get());
				context.clear_frame();
				SDL_Delay(1);
			}
		};

		if constexpr (std::is_constructible_v<GameState, app_context &>)
		{
			auto state = GameState(context);

			return run_loop(state);
		}
		else
		{
			auto state = GameState();

			return run_loop(state);
		}
	}
};
