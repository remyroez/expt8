
#include <SDL.h>
#include <stdio.h>

namespace {

constexpr int logical_width = 256;
constexpr int logical_height = 244;
constexpr int default_scale = 2;
constexpr int default_width = (logical_width * default_scale);
constexpr int default_height = (logical_height * default_scale);
constexpr Uint32 fps = 60;
constexpr Uint32 ms_frame = (1000U / fps);

auto inline print_sdl_error() {
	return printf("%s", SDL_GetError());
}

} // namespace

int main(int argc, char **argv) {
	if (auto init = SDL_Init(SDL_INIT_EVERYTHING); init < 0) {
		print_sdl_error();

	} else if (auto *window = SDL_CreateWindow(
		"expt8",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		default_width, default_height,
		SDL_WINDOW_RESIZABLE
	); window == nullptr) {
		print_sdl_error();
		SDL_Quit();

	} else if (auto *renderer = SDL_CreateRenderer(
		window, -1, (SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED)
	); renderer == nullptr) {
		print_sdl_error();
		SDL_DestroyWindow(window);
		SDL_Quit();

	} else {
		SDL_SetWindowMinimumSize(window, logical_width, logical_height);
		SDL_RenderSetLogicalSize(renderer, logical_width, logical_height);
		SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

		bool running = true;
		while (running) {
			auto initial_ms = SDL_GetTicks();

			SDL_Event event{};
			while (SDL_PollEvent(&event) != 0) {
				if (event.type == SDL_QUIT) {
					running = false;

				} else if (event.type == SDL_WINDOWEVENT) {
					if ((event.window.type == SDL_WINDOWEVENT_CLOSE) && (event.window.windowID == SDL_GetWindowID(window))) {
						running = false;
					}
				}
			}

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
			SDL_RenderClear(renderer);

			SDL_Rect rect{ 0, 0, logical_width, logical_height };
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderFillRect(renderer, &rect);

			SDL_RenderPresent(renderer);

			auto elapsed_ms = (SDL_GetTicks() - initial_ms);
			if (elapsed_ms < ms_frame) SDL_Delay(ms_frame - elapsed_ms);
		}

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
	return 0;
}
