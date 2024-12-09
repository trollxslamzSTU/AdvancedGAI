#include "GameScreen_Chess.h"

namespace
{
	struct game_loop final
	{
		static constexpr SDL_Color const white = sdl_game::rgb(0.852f, 0.929f, 0.643f);

		static constexpr SDL_Color const black = sdl_game::rgb(0.384f, 0.377f, 0.722f);

		bool on_ready(sdl_game::app_context & context)
		{
			_game_screen = std::make_unique<GameScreen_Chess>(context);

			return true;
		}

		void on_event(sdl_game::app_context const & context, SDL_Event const & event)
		{
			_game_screen->Update(1.f / 60.f, event);
		}

		void on_render(sdl_game::app_context & context)
		{
			context.render_clear();
			_game_screen->Render(context);
		}

		private:
		std::unique_ptr<GameScreen_Chess> _game_screen;
	};
}

int main(int argc, char * argv[])
{
	return sdl_game::init<game_loop>(
	{
		.title = "Chess",
		.width = 1280,
		.height = 720,
	});
}
