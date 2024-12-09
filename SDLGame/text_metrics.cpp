#include "sdl_game.h"

using namespace sdl_game;

text_metrics text_metrics::measure_text(sprite_font const & font, std::string_view const & text)
{
	float total_width = 0;
    float total_height = font.line_height;
    float current_line_width = 0;
    size_t line_count = 1;

	for (char c : text)
	{
		switch (c)
		{
			case '\n':
			{
				total_height += font.line_height;
                total_width = std::max(total_width, current_line_width);
                current_line_width = 0.0f;
                line_count++;
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

                current_line_width += static_cast<float>(glyph.advance);
			}
			break;
		}
	}

    return
    {
        .width = std::max(total_width, current_line_width),
        .height = total_height,
        .line_count = line_count,
    };
}
