#include "sdl_game.h"

using namespace sdl_game;

void app_context::clear_frame()
{
	_key_buttons.clear_frame();
	_mouse_buttons.clear_frame();
}

void app_context::consume_event(SDL_Event const & event)
{
	switch (event.type)
	{
		case SDL_QUIT:
		{
			quit();
		}
		break;

		case SDL_KEYDOWN:
		{
			if (!event.key.repeat)
			{
				_key_buttons.set(event.key.keysym.scancode, true);
			}
		}
		break;

		case SDL_KEYUP:
		{
			if (!event.key.repeat)
			{
				_key_buttons.set(event.key.keysym.scancode, false);
			}
		}
		break;

		case SDL_MOUSEMOTION:
		{
			_mouse_position =
			{
				.x = static_cast<float>(event.motion.x),
				.y = static_cast<float>(event.motion.y),
			};
		}
		break;

		case SDL_MOUSEBUTTONDOWN:
		{
			_mouse_buttons.set(event.button.button, true);
		}
		break;

		case SDL_MOUSEBUTTONUP:
		{
			_mouse_buttons.set(event.button.button, false);
		}
		break;

		default: break;
	}
}

SDL_Point app_context::key_axis(key_axis_mapping const & mapping) const
{
	return
	{
		.x = static_cast<int>(_key_buttons.is_held(mapping.x_pos)) - static_cast<int>(_key_buttons.is_held(mapping.x_neg)),
		.y = static_cast<int>(_key_buttons.is_held(mapping.y_pos)) - static_cast<int>(_key_buttons.is_held(mapping.y_neg)),
	};
}

std::shared_ptr<SDL_Texture> app_context::load_texture(std::string const & image_path, texture_filtering const & filtering)
{
	switch (filtering)
	{
		case texture_filtering_nearest:
		{
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
		}
		break;

		case texture_filtering_linear:
		{
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		}
		break;

		default: unreachable();
	}

	auto const bitmap = std::unique_ptr<SDL_Surface, sdl_deleter>(IMG_Load(image_path.c_str()));

	if (!bitmap) [[unlikely]]
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load %s", image_path.c_str());
	}

	return std::shared_ptr<SDL_Texture>(SDL_CreateTextureFromSurface(_renderer, bitmap.get()), sdl_deleter());
}

sprite_font app_context::load_sprite_font(std::string const & font_path, uint8_t const & font_size)
{
	sprite_font font;

	if (auto const ttf = std::unique_ptr<TTF_Font, sdl_deleter>(TTF_OpenFont(font_path.c_str(), font_size)))
	{
		constexpr SDL_Color const base_color =
		{
			.r = UINT8_MAX,
			.g = UINT8_MAX,
			.b = UINT8_MAX,
			.a = UINT8_MAX,
		};

		std::array<std::unique_ptr<SDL_Surface, sdl_deleter>, font.glyphs.size()> glyph_symbols;
		constexpr int const glyph_padding = 2;
		int glyph_width = 0, glyph_height = 0;

		for (size_t i = 0; i < glyph_symbols.size(); i += 1)
		{
			auto const c = static_cast<char>(i + sprite_font::glyph_head);
			auto glyph_symbol = std::unique_ptr<SDL_Surface, sdl_deleter>(TTF_RenderGlyph_Blended(ttf.get(), c, base_color));

			if (!glyph_symbol)
			{
				continue;
			}

			glyph_width = std::max(glyph_width, glyph_symbol->w + glyph_padding);
			glyph_height = std::max(glyph_height, glyph_symbol->h + glyph_padding);
			glyph_symbols[i] = std::move(glyph_symbol);
		}

		int const atlas_width = glyph_width * 16;
		int const atlas_height = glyph_height * 8;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		if (auto const atlas = std::unique_ptr<SDL_Surface, sdl_deleter>(SDL_CreateRGBSurface(0, atlas_width, atlas_height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff)))
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
		if (auto const atlas = std::unique_ptr<SDL_Surface, sdl_deleter>(SDL_CreateRGBSurface(0, atlas_width, atlas_height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)))
#endif
		{
			SDL_FillRect(atlas.get(), nullptr, SDL_MapRGBA(atlas->format, 0, 0, 0, 0));

			for (size_t i = 0; i < glyph_symbols.size(); i += 1)
			{
				SDL_Surface * const glyph_symbol = glyph_symbols[i].get();

				if (!glyph_symbol)
				{
					continue;
				}

				SDL_Rect region =
				{
					.x = static_cast<int>(i % 16) * glyph_width,
					.y = static_cast<int>(i / 16) * glyph_height,
					.w = glyph_height,
					.h = glyph_width,
				};

				SDL_BlitSurface(glyph_symbol, NULL, atlas.get(), &region);

				auto const c = static_cast<char>(i + sprite_font::glyph_head);
				int advance = 0;

				TTF_GlyphMetrics(ttf.get(), c, nullptr, nullptr, nullptr, nullptr, &advance);

				font.glyphs[i] =
				{
					.region = region,
					.advance = static_cast<float>(advance),
				};
			}

			font.atlas = std::shared_ptr<SDL_Texture>(SDL_CreateTextureFromSurface(_renderer, atlas.get()), sdl_deleter());
			font.line_height = static_cast<float>(TTF_FontHeight(ttf.get()));
		}
	}

	if (!font) [[unlikely]]
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load %s", font_path.c_str());
	}

	return font;
}

texture_info app_context::query_renderer() const
{
	int width = 0, height = 0;

	SDL_GetRendererOutputSize(_renderer, &width, &height);

	return
	{
		.width = static_cast<uint16_t>(width),
		.height = static_cast<uint16_t>(height)
	};
}

void app_context::quit()
{
	_has_quit = true;
}

texture_info app_context::query_texture(std::shared_ptr<SDL_Texture> const & texture) const
{
	int width = 0, height = 0;

	SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height);

	return
	{
		.width = static_cast<uint16_t>(width),
		.height = static_cast<uint16_t>(height),
	};
}

void app_context::render_atlas(std::shared_ptr<SDL_Texture> const & atlas, SDL_FPoint const & origin, render_atlas_options const & options)
{
	SDL_SetTextureColorMod(atlas.get(), options.tint_color.r, options.tint_color.g, options.tint_color.b);

	SDL_FRect const target =
	{
		.x = origin.x,
		.y = origin.y,
		.w = static_cast<float>(options.region.w),
		.h = static_cast<float>(options.region.h),
	};

	SDL_RenderCopyExF(_renderer, atlas.get(), &options.region, &target, options.rotation, options.center ? &*options.center : nullptr, SDL_FLIP_NONE);
}

void app_context::render_clear(SDL_Color const & clear_color)
{
	SDL_SetRenderDrawColor(_renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	SDL_RenderClear(_renderer);
}

void app_context::render_sheet(std::shared_ptr<SDL_Texture> const & sheet, SDL_FPoint origin, render_sheet_options const & options)
{
	SDL_SetTextureColorMod(sheet.get(), options.tint_color.r, options.tint_color.g, options.tint_color.b);

	int sheet_width, sheet_height = 0;

	SDL_QueryTexture(sheet.get(), nullptr, nullptr, &sheet_width, &sheet_height);

	int const frame_width = sheet_width / options.columns;
	int const frame_height = sheet_height / options.rows;

	SDL_FRect const target =
	{
		.x = origin.x,
		.y = origin.y,
		.w = static_cast<float>(frame_width),
		.h = static_cast<float>(frame_height),
	};

	auto const row = static_cast<int>(options.frame / options.columns);
	auto const column = static_cast<int>(options.frame % options.columns);

	SDL_Rect const region =
	{
		.x = column * frame_width,
		.y = row * frame_height,
		.w = frame_width,
		.h = frame_height,
	};

	SDL_RenderCopyExF(_renderer, sheet.get(), &region, &target, options.rotation, options.center ? &*options.center : nullptr, SDL_FLIP_NONE);
}

void app_context::render_sprite(std::shared_ptr<SDL_Texture> const & sprite, SDL_FPoint origin, render_sprite_options const & options)
{
	SDL_SetTextureColorMod(sprite.get(), options.tint_color.r, options.tint_color.g, options.tint_color.b);

	texture_info const info = query_texture(sprite);

	SDL_FRect const rect =
	{
		.x = origin.x,
		.y = origin.y,
		.w = static_cast<float>(info.width),
		.h = static_cast<float>(info.height),
	};

	SDL_RenderCopyExF(_renderer, sprite.get(), nullptr, &rect, options.rotation, nullptr, SDL_FLIP_NONE);
}

void app_context::render_font(sprite_font const & font, SDL_FPoint const & origin, std::string_view const & text, render_font_options const & options)
{
	SDL_SetTextureColorMod(font.atlas.get(), options.fill_color.r, options.fill_color.g, options.fill_color.b);

	SDL_FRect target =
	{
		.x = origin.x,
		.y = origin.y,
	};

	for (char c : text)
	{
		switch (c)
		{
			case '\n':
			{
				target.y += font.line_height;
				target.x = 0;
			}
			break;

			default:
			{
				if (c < sprite_font::glyph_head || c > sprite_font::glyph_tail)
				{
					continue;
				}

				size_t const index = static_cast<size_t>(c) - sprite_font::glyph_head;
				sprite_font::glyph_metrics const & glyph = font.glyphs[index];

				target.w = static_cast<float>(glyph.region.w);
				target.h = static_cast<float>(glyph.region.h);

				SDL_RenderCopyF(_renderer, font.atlas.get(), &glyph.region, &target);

				target.x += glyph.advance;
			}
			break;
		}
	}
}

void app_context::render_line(SDL_Color const & color, SDL_FPoint const & origin, SDL_FPoint const & target)
{
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLineF(_renderer, origin.x, origin.y, target.x, target.y);
}

void app_context::render_point(SDL_Color const & point_color, SDL_FPoint const & point)
{
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(_renderer, point_color.r, point_color.g, point_color.b, point_color.a);
	SDL_RenderDrawPointF(_renderer, point.x, point.y);
}

void app_context::render_rect_outline(SDL_Color const & color, float const & width, SDL_FRect const & rect)
{
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, color.a);

	float const actual_width = std::ceil(std::min(std::min(width, rect.w), rect.h));

	if (actual_width == 1.f)
	{
		SDL_RenderDrawRectF(_renderer, &rect);
	}
	else
	{
		SDL_FRect banding_rect = rect;

		for (float i = 0; i < actual_width; i += 1)
		{
			SDL_RenderDrawRectF(_renderer, &banding_rect);

			banding_rect.x += 1;
			banding_rect.y += 1;
			banding_rect.w -= 2;
			banding_rect.h -= 2;
		}
	}
}

void app_context::render_rect_solid(SDL_Color const & solid_color, SDL_FRect const & rect)
{
	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(_renderer, solid_color.r, solid_color.g, solid_color.b, solid_color.a);
	SDL_RenderFillRectF(_renderer, &rect);
}

void app_context::set_render_clip(std::optional<SDL_Rect> const & clip_rect)
{
	SDL_RenderSetClipRect(_renderer, clip_rect ? &*clip_rect : nullptr);
}
