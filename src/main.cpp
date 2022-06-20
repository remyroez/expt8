
#include <stdio.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string_view>
#include <filesystem>
#include <array>
#include <random>
#include <cmath>
#include <numbers>

#include <SDL.h>

#include <wasm3.h>
#include <m3_env.h>

#include "runtime.h"

#define EXPT8_WASM (0)

namespace {

constexpr int logical_width = 256;
constexpr int logical_height = 240;
constexpr int default_scale = 2;
constexpr int default_width = (logical_width * default_scale);
constexpr int default_height = (logical_height * default_scale);
constexpr Uint64 fps = 60;
constexpr Uint64 ms_frame = (1000LLU / fps);
constexpr uint32_t memory_size = (1024 * 64) * 2;

using framebuffer = std::array<expt8::pixel_t, logical_width * logical_height>;

constexpr uint8_t input_right = 0b00000001;
constexpr uint8_t input_left = 0b00000010;
constexpr uint8_t input_down = 0b00000100;
constexpr uint8_t input_up = 0b00001000;
constexpr uint8_t input_a = 0b00010000;
constexpr uint8_t input_b = 0b00100000;
constexpr uint8_t input_start = 0b01000000;
constexpr uint8_t input_select = 0b10000000;

uint8_t input_state_last = 0;
uint8_t input_state = 0;

SDL_Renderer *renderer = nullptr;

auto inline print_sdl_error() {
	return SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
}

int sum(int a, int b) {
	return a + b;
}

m3ApiRawFunction(wasm_sum) {
	m3ApiReturnType(int);
	m3ApiGetArg(int, a);
	m3ApiGetArg(int, b);
	m3ApiReturn(sum(a, b));
}

void *ext_memcpy(void *dst, const void *arg, int32_t size) {
	return memcpy(dst, arg, (size_t)size);
}

m3ApiRawFunction(wasm_ext_memcpy) {
	m3ApiReturnType(void *);
	m3ApiGetArgMem(void *, dst);
	m3ApiGetArgMem(const void *, arg);
	m3ApiGetArg(int32_t, size);
	m3ApiReturn(ext_memcpy(dst, arg, size));
}

m3ApiRawFunction(wasm_draw_color) {
	m3ApiReturnType(int);
	m3ApiGetArg(unsigned int, r);
	m3ApiGetArg(unsigned int, g);
	m3ApiGetArg(unsigned int, b);
	m3ApiReturn(SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF));
}

m3ApiRawFunction(wasm_draw_rect) {
	m3ApiReturnType(int);
	m3ApiGetArg(int, x);
	m3ApiGetArg(int, y);
	m3ApiGetArg(int, w);
	m3ApiGetArg(int, h);
	SDL_Rect rect{ x, y, w, h };
	m3ApiReturn(SDL_RenderFillRect(renderer, &rect));
}

m3ApiRawFunction(wasm_input) {
	m3ApiReturnType(int);
	m3ApiGetArg(int, id);
	int on = 0;
	if (id < 0) {
		// 不正
	} else if (id >= 8) {
		// 不正
	} else {
		on = (input_state & (1 << id)) ? 1 : 0;
	}
	m3ApiReturn(on);
}

m3ApiRawFunction(wasm_press) {
	m3ApiReturnType(int);
	m3ApiGetArg(int, id);
	int on = 0;
	if (id < 0) {
		// 不正
	} else if (id >= 8) {
		// 不正
	} else {
		auto last = ((input_state_last & (1 << id)) ? 1 : 0);
		auto current = ((input_state & (1 << id)) ? 1 : 0);
		on = ((last == 0) && (current != 0)) ? 1 : 0;
	}
	m3ApiReturn(on);
}

std::filesystem::path current_wasm;

IM3Environment environment = nullptr;
IM3Runtime runtime = nullptr;
IM3Module module = nullptr;

IM3Function test = nullptr;
IM3Function test_memcpy = nullptr;
IM3Function test_counter_get = nullptr;
IM3Function test_counter_inc = nullptr;
IM3Function test_counter_add = nullptr;
IM3Function update = nullptr;

void finalize_m3() {
	test = nullptr;
	test_memcpy = nullptr;
	test_counter_get = nullptr;
	test_counter_inc = nullptr;
	test_counter_add = nullptr;
	update = nullptr;
	{
		module = nullptr;
	}
	if (runtime) {
		m3_FreeRuntime(runtime);
		runtime = nullptr;
	}
	if (environment) {
		m3_FreeEnvironment(environment);
		environment = nullptr;
	}
}

bool setup_m3(uint32_t stack_size) {
	bool succeeded = false;
	if (environment = m3_NewEnvironment(); environment == nullptr) {

	} else if (runtime = m3_NewRuntime(environment, stack_size, nullptr); runtime == nullptr) {

	} else {
		runtime->memory.maxPages = 1;
		ResizeMemory(runtime, 1);
		succeeded = true;
	}
	return succeeded;
}

bool initialize_m3(const uint8_t *const file_data, uint32_t file_size) {
	bool succeeded = false;
	finalize_m3();
	if (!setup_m3(memory_size)) {

	} else if (auto parse_result = m3_ParseModule(environment, &module, file_data, file_size)) {

	} else if (auto load_result = m3_LoadModule(runtime, module)) {

	} else {
		succeeded = true;
	}
	if (!succeeded) finalize_m3();
	return succeeded;
}

bool setup_wasm(const std::filesystem::path &file_path) {
	bool succeeded = false;

	if (std::ifstream file(file_path, std::ios::binary | std::ios::in); !file.is_open()) {

	} else {
		file.unsetf(std::ios::skipws);
		std::vector<uint8_t> buffer;
		std::copy(
			std::istream_iterator<uint8_t>(file),
			std::istream_iterator<uint8_t>(),
			std::back_inserter(buffer)
		);
		if (!initialize_m3(buffer.data(), buffer.size())) {

		} else {
			module->memoryImported = true;

			current_wasm = file_path;

			m3_LinkRawFunction(module, "*", "sum", "i(ii)", wasm_sum);
			m3_LinkRawFunction(module, "*", "ext_memcpy", "*(**i)", wasm_ext_memcpy);
			m3_LinkRawFunction(module, "*", "draw_color", "i(iii)", wasm_draw_color);
			m3_LinkRawFunction(module, "*", "draw_rect", "i(iiii)", wasm_draw_rect);
			m3_LinkRawFunction(module, "*", "input", "i(i)", wasm_input);
			m3_LinkRawFunction(module, "*", "press", "i(i)", wasm_press);
			m3_FindFunction(&test, runtime, "test");
			m3_FindFunction(&test_memcpy, runtime, "test_memcpy");
			m3_FindFunction(&test_counter_get, runtime, "test_counter_get");
			m3_FindFunction(&test_counter_inc, runtime, "test_counter_inc");
			m3_FindFunction(&test_counter_add, runtime, "test_counter_add");
			m3_FindFunction(&update, runtime, "update");

			M3ErrorInfo error;
			m3_GetErrorInfo(runtime, &error);
			if (error.result) {
				SDL_Log("Error in load: %s: %s\n", error.result, error.message);
			}

			if (test) {
				m3_CallV(test, 20, 10);
				int result = 0;
				m3_GetResultsV(test, &result);
				SDL_Log("test -> %d", result);
			}

			if (test_memcpy) {
				m3_CallV(test_memcpy);
				int64_t result = 0;
				m3_GetResultsV(test_memcpy, &result);
				SDL_Log("test_memcpy -> %llx", result);
			}

			if (test_counter_get) {
				m3_CallV(test_counter_get);
				int result = 0;
				m3_GetResultsV(test_counter_get, &result);
				SDL_Log("test_counter_get -> %d", result);
			}

			if (test_counter_inc && test_counter_get) {
				m3_CallV(test_counter_inc);
				m3_CallV(test_counter_get);
				int result = 0;
				m3_GetResultsV(test_counter_get, &result);
				SDL_Log("test_counter_inc -> test_counter_get -> %d", result);
			}

			if (test_counter_add && test_counter_get)
			{
				m3_CallV(test_counter_add, 42);
				m3_CallV(test_counter_get);
				int result = 0;
				m3_GetResultsV(test_counter_get, &result);
				SDL_Log("test_counter_add -> test_counter_get -> %d", result);
			}

			succeeded = true;
		}
	}
	return succeeded;
}

} // namespace

int main(int argc, char **argv) {
#if EXPT8_WASM
	{
		std::filesystem::path file_path = "boot.wasm";
		for (int i = 1; i < argc; ++i) {
			auto arg = std::string_view(argv[i]);
			if (arg.starts_with("-")) continue;
			file_path = arg;
		}
		if (!setup_wasm(file_path)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "wasm3 error");
		}
	}
#endif
	
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

	} else if (renderer = SDL_CreateRenderer(
		window, -1, (SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED)
	); renderer == nullptr) {
		print_sdl_error();
		SDL_DestroyWindow(window);
		SDL_Quit();

	} else {
		SDL_SetWindowMinimumSize(window, logical_width, logical_height);
		SDL_RenderSetLogicalSize(renderer, logical_width, logical_height);
		SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

		std::vector<Uint8> KeyboardState;
		{
			int Num = 0;
			SDL_GetKeyboardState(&Num);
			KeyboardState.resize(Num);
		}

		expt8::runtime runtime;

		// dummy bg color
		runtime.set_background_color(0x00);

		// dummy bg
		{
			expt8::pixel_t tile[] = {
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 2, 2, 2, 2, 2, 2, 1,
				1, 2, 3, 3, 3, 3, 2, 1,
				1, 2, 3, 0, 0, 3, 2, 1,
				1, 2, 3, 0, 0, 3, 2, 1,
				1, 2, 3, 3, 3, 3, 2, 1,
				1, 2, 2, 2, 2, 2, 2, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
			};
			runtime.write_pattern(0, 0, tile);
		}
		{
			expt8::pixel_t tile[] = {
				1, 1, 1, 1, 1, 1, 1, 1,
				2, 2, 2, 2, 2, 2, 2, 2,
				3, 3, 3, 3, 3, 3, 3, 3,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				3, 3, 3, 3, 3, 3, 3, 3,
				2, 2, 2, 2, 2, 2, 2, 2,
				1, 1, 1, 1, 1, 1, 1, 1,
			};
			runtime.write_pattern(0, 1, tile);
		}
		{
			expt8::pixel_t tile[] = {
				1, 3, 3, 3, 3, 3, 3, 2,
				3, 1, 0, 0, 0, 0, 2, 3,
				3, 0, 1, 0, 0, 2, 0, 3,
				3, 0, 0, 1, 2, 0, 0, 3,
				3, 0, 0, 2, 1, 0, 0, 3,
				3, 0, 2, 0, 0, 1, 0, 3,
				3, 2, 0, 0, 0, 0, 1, 3,
				2, 3, 3, 3, 3, 3, 3, 1,
			};
			runtime.write_pattern(0, 2, tile);
		}
		{
			expt8::pixel_t tile[] = {
				3, 0, 1, 1, 1, 1, 0, 3,
				0, 1, 0, 0, 0, 0, 1, 0,
				1, 0, 2, 2, 2, 2, 0, 1,
				1, 0, 2, 0, 0, 2, 0, 1,
				1, 0, 2, 0, 0, 2, 0, 1,
				1, 0, 2, 2, 2, 2, 0, 1,
				0, 1, 0, 0, 0, 0, 1, 0,
				3, 0, 1, 1, 1, 1, 0, 3,
			};
			runtime.write_pattern(0, 3, tile);
		}
		{
			runtime.set_background_palette(0, 0x00, 0x0F, 0x21, 0x26);
			runtime.set_background_palette(1, 0x00, 0x29, 0x13, 0x0F);
		}
		{
			int p = 0;
			for (int y = 0; y < expt8::tile_table::height; ++y) {
				for (int x = 0; x < expt8::tile_table::width; ++x) {
					runtime.set_tile(0, x, y, x % 2);
					if ((x % 2 == 0) && (y % 2 == 0)) runtime.set_tile_palette(0, x, y, p++ % 2);
				}
			}
		}
		{
			int p = 0;
			for (int y = 0; y < expt8::tile_table::height; ++y) {
				for (int x = 0; x < expt8::tile_table::width; ++x) {
					runtime.set_tile(1, x, y, (x == 0 || x == (expt8::tile_table::width - 1) || y == 0 || y == (expt8::tile_table::height - 1)) ? 2 : 0);
				}
			}
		}
		{
			int p = 0;
			for (int y = 0; y < expt8::tile_table::height; ++y) {
				for (int x = 0; x < expt8::tile_table::width; ++x) {
					runtime.set_tile(2, x, y, (x == 0 || x == (expt8::tile_table::width - 1) || y == 0 || y == (expt8::tile_table::height - 1)) ? 3 : 0);
				}
			}
		}

		// dummy sprite
		{
			expt8::pixel_t tile[] = {
				0, 0, 0, 1, 1, 0, 0, 0,
				0, 0, 1, 1, 1, 1, 0, 0,
				0, 1, 1, 1, 1, 1, 1, 0,
				1, 1, 1, 3, 3, 1, 1, 1,
				2, 2, 2, 3, 3, 2, 2, 2,
				0, 2, 2, 2, 2, 2, 2, 0,
				0, 0, 2, 2, 2, 2, 0, 0,
				0, 0, 0, 2, 2, 0, 0, 0,
			};
			runtime.write_pattern(1, 0, tile);
			runtime.set_sprite_pattern_table(1);
			runtime.set_sprite(
				0,
				0, 0,
				0,
				0
			);
			runtime.set_sprite_palette(0, 0x00, 0x15, 0x29, 0x30);
		}

		framebuffer fb{ 0 };
		std::array<Uint32, 256> palette{ 0 };

		// dummy framebuffer
		{
			for (int y = 0; y < logical_height; ++y) {
				for (int x = 0; x < logical_width; ++x) {
					fb[y * logical_width + x] = x / 16 + y / 16 * 16;
				}
			}
		}

		// palette
		{
			constexpr Uint32 rgb_colors[] = {
				0x757575, 0x271B8F, 0x0000AB, 0x47009F, 0x8F0077, 0xAB0013, 0xA70000, 0x7F0B00,
				0x432F00, 0x004700, 0x005100, 0x003F17, 0x1B3F5F, 0x000000, 0x000000, 0x000000,

				0xBCBCBC, 0x0073EF, 0x233BEF, 0x8300F3, 0xBF00BF, 0xE7005B, 0xDB2B00, 0xCB4F0F,
				0x8B7300, 0x009700, 0x00AB00, 0x00933B, 0x00838B, 0x000000, 0x000000, 0x000000,

				0xFFFFFF, 0x3FBFFF, 0x5F73FF, 0xA78BFD, 0xF77BFF, 0xFF77B7, 0xFF7763, 0xFF9B3B,
				0xF3BF3F, 0x83D313, 0x4FDF4B, 0x58F898, 0x00EBDB, 0x757575, 0x000000, 0x000000,

				0xFFFFFF, 0xABE7FF, 0xC7D7FF, 0xD7CBFF, 0xFFC7FF, 0xFFC7DB, 0xFFBFB3, 0xFFDBAB,
				0xFFE7A3, 0xE3FFA3, 0xABF3BF, 0xB3FFCF, 0x9FFFF3, 0xBCBCBC, 0x000000, 0x000000,
			};
			constexpr size_t num_colors = sizeof(rgb_colors) / sizeof(rgb_colors[0]);

			auto *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
			auto pal = std::span{ palette.data(), num_colors };

			for (int i = 0; i < pal.size(); ++i) {
				auto color = rgb_colors[i];
				pal[i] = SDL_MapRGB(format, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
			}
		}

		auto *screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, logical_width, logical_height);

		bool grayscale = false;
		bool fullscreen = false;
		int raster = 0;

		runtime.ppu().set_callback(
			[&](int x, int y) {
				if (y < 32 || x < logical_width / 2) {
					runtime.set_scroll(0, 0);

				} else {
					auto base = (y + raster) % logical_height;
					auto s = std::sin(std::numbers::pi * 2 * (static_cast<double>(base) / static_cast<double>(logical_height - 1))) ;
					runtime.set_scroll(s * 32, 0);
				}
			},
			expt8::picture_processing_unit::always
		);

		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_int_distribution<> distw(0, logical_width - expt8::pattern::width);
		std::uniform_int_distribution<> disth(0, logical_height - expt8::pattern::height);
		std::uniform_int_distribution<> disti(0, 1);

		struct entity {
			int spr_x = 0, spr_y = 0;
			int vel_x = 1, vel_y = 1;
		};
		std::array<entity, 64> entities;
		for (int i = 0; i < entities.size(); ++i) {
			entities[i].spr_x = distw(mt);
			entities[i].spr_y = disth(mt);
			entities[i].vel_x = disti(mt) == 0 ? -1 : 1;
			entities[i].vel_y = disti(mt) == 0 ? -1 : 1;
		}
		expt8::coordinate_t scroll_x = 0;
		expt8::coordinate_t scroll_y = 0;

		const auto unit = SDL_GetPerformanceFrequency() / ::fps;
		auto before_ticks = SDL_GetPerformanceCounter();
		Uint64 lag = 0;
		int max_skip = 2;

		bool running = true;
		while (running) {
			auto current_ticks = SDL_GetPerformanceCounter();
			auto elapsed_ticks = current_ticks - before_ticks;
			before_ticks = current_ticks;
			lag += elapsed_ticks;

			SDL_Event event{};
			while (SDL_PollEvent(&event) != 0) {
				if (event.type == SDL_QUIT) {
					running = false;

				} else if (event.type == SDL_WINDOWEVENT) {
					if (event.window.windowID != SDL_GetWindowID(window)) {
						//
					} else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						running = false;
					}
				}
			}

			auto *CurrentKeyboardState = SDL_GetKeyboardState(nullptr);

#if EXPT8_WASM
			// reload wasm
			if (!KeyboardState[SDL_SCANCODE_F5] && CurrentKeyboardState[SDL_SCANCODE_F5]) {
				setup_wasm(current_wasm);
			}
#endif

			if (!KeyboardState[SDL_SCANCODE_F11] && CurrentKeyboardState[SDL_SCANCODE_F11]) {
				if (fullscreen) {
					SDL_SetWindowFullscreen(window, 0);

				} else {
					SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
				}
				fullscreen = !fullscreen;
			}

			::input_state = 0;
			if (CurrentKeyboardState[SDL_SCANCODE_RIGHT]) ::input_state |= input_right;
			if (CurrentKeyboardState[SDL_SCANCODE_LEFT])  ::input_state |= input_left;
			if (CurrentKeyboardState[SDL_SCANCODE_DOWN])  ::input_state |= input_down;
			if (CurrentKeyboardState[SDL_SCANCODE_UP])    ::input_state |= input_up;
			if (CurrentKeyboardState[SDL_SCANCODE_Z]) ::input_state |= input_a;
			if (CurrentKeyboardState[SDL_SCANCODE_X]) ::input_state |= input_b;
			if (CurrentKeyboardState[SDL_SCANCODE_RETURN]) ::input_state |= input_start;
			if (CurrentKeyboardState[SDL_SCANCODE_SPACE]) ::input_state |= input_select;
			
			int count = 0;
			bool skip = (lag / unit) > 1;
			while (lag >= unit) {
				if (::input_state & ::input_right) scroll_x += 1;
				if (::input_state & ::input_left) scroll_x -= 1;
				if (::input_state & ::input_down) scroll_y += 1;
				if (::input_state & ::input_up) scroll_y -= 1;
				//runtime.set_scroll(scroll_x, scroll_y);
				raster = (raster + 1) % logical_height;

				for (int i = 0; i < entities.size(); ++i) {
					auto &spr_x = entities[i].spr_x;
					auto &spr_y = entities[i].spr_y;
					auto &vel_x = entities[i].vel_x;
					auto &vel_y = entities[i].vel_y;
					spr_x += vel_x;
					spr_y += vel_y;
					if (spr_x < 0) {
						spr_x = 0;
						vel_x = -vel_x;

					} else if ((spr_x + expt8::pattern::width) >= logical_width) {
						spr_x = logical_width - expt8::pattern::width;
						vel_x = -vel_x;
					}
					if (spr_y < 0) {
						spr_y = 0;
						vel_y = -vel_y;

					} else if ((spr_y + expt8::pattern::height) >= logical_height) {
						spr_y = logical_height - expt8::pattern::height;
						vel_y = -vel_y;
					}
					runtime.set_sprite(
						i,
						spr_x, spr_y,
						0,
						0,
						(i >= 32) ? expt8::sprite::priority_back : 0
					);
				}

				runtime.render_picture(std::span{ fb }, logical_width, logical_height);
				lag -= unit;

				count++;
				if (count > max_skip) {
					lag = 0;
					break;
				}

				if (skip) {
					while (SDL_PollEvent(&event) != 0) {
						if (event.type == SDL_QUIT) {
							running = false;

						} else if (event.type == SDL_WINDOWEVENT) {
							if (event.window.windowID != SDL_GetWindowID(window)) {
								//
							} else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
								running = false;
							}
						}
					}
					SDL_Delay(1);
				}
			}

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
			SDL_RenderClear(renderer);

#if 0
			{
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_Rect rect{ 0, 0, logical_width, logical_height };
				SDL_RenderFillRect(renderer, &rect);
			}
#else

			{
				auto *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
				void *pixels = nullptr;
				int pitch = 0;
				if (SDL_LockTexture(screen, nullptr, &pixels, &pitch) >= 0) {
					for (int y = 0; y < logical_height; ++y) {
						auto *dst = (Uint32 *)((Uint8 *)pixels + y * pitch);
						for (int x = 0; x < logical_width; ++x) {
							auto index_color = fb[y * logical_width + x];
							//if (grayscale && ((index_color & 0x0F) <= 0x0C)) index_color = index_color & ~0x0F;
							dst[x] = palette[index_color];
						}
					}
					SDL_UnlockTexture(screen);
				}
				SDL_RenderCopy(renderer, screen, nullptr, nullptr);
			}
#endif

#if EXPT8_WASM
			if (update) {
				m3_CallV(update);
				int result = 0;
				m3_GetResultsV(update, &result);
			}
#endif
			SDL_RenderPresent(renderer);

#if 1//EXPT8_WASM
			std::copy(CurrentKeyboardState, &CurrentKeyboardState[SDL_NUM_SCANCODES], KeyboardState.begin());
#endif
			SDL_Delay(1);
		}

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	finalize_m3();
	return 0;
}
