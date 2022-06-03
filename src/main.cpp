
#include <stdio.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <string_view>
#include <filesystem>

#include <SDL.h>

#include <wasm3.h>
#include <m3_env.h>

namespace {

constexpr int logical_width = 256;
constexpr int logical_height = 240;
constexpr int default_scale = 2;
constexpr int default_width = (logical_width * default_scale);
constexpr int default_height = (logical_height * default_scale);
constexpr Uint32 fps = 60;
constexpr Uint32 ms_frame = (1000U / fps);
constexpr uint32_t memory_size = (1024 * 64) * 2;

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

bool initialize_m3(const uint8_t *const file_data, uint32_t file_size) {
	bool succeeded = false;
	finalize_m3();
	if (environment = m3_NewEnvironment(); environment == nullptr) {

	} else if (runtime = m3_NewRuntime(environment, memory_size, nullptr); runtime == nullptr) {

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
	std::filesystem::path file_path = "boot.wasm";

	for (int i = 1; i < argc; ++i) {
		auto arg = std::string_view(argv[i]);
		if (arg.starts_with("-")) continue;
		file_path = arg;
	}

	if (!setup_wasm(file_path)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "wasm3 error");

	} else if (auto init = SDL_Init(SDL_INIT_EVERYTHING); init < 0) {
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

		bool running = true;
		while (running) {
			auto initial_ms = SDL_GetTicks();
			bool press = false;
			SDL_Event event{};
			while (SDL_PollEvent(&event) != 0) {
				if (event.type == SDL_QUIT) {
					running = false;

				} else if (event.type == SDL_WINDOWEVENT) {
					if ((event.window.type == SDL_WINDOWEVENT_CLOSE) && (event.window.windowID == SDL_GetWindowID(window))) {
						running = false;
					}

				} else if (event.type == SDL_KEYDOWN) {
					if ((event.key.state == SDL_PRESSED) && (event.key.windowID == SDL_GetWindowID(window)) && (event.key.repeat == 0)) {
						press = true;
					}
				}
			}

			auto *CurrentKeyboardState = SDL_GetKeyboardState(nullptr);

			// reload wasm
			if (!KeyboardState[SDL_SCANCODE_F5] && CurrentKeyboardState[SDL_SCANCODE_F5]) {
				setup_wasm(current_wasm);
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

			SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0);
			SDL_RenderClear(renderer);

			{
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_Rect rect{ 0, 0, logical_width, logical_height };
				SDL_RenderFillRect(renderer, &rect);
			}

			if (update) {
				m3_CallV(update);
				int result = 0;
				m3_GetResultsV(update, &result);
			}

			SDL_RenderPresent(renderer);

			std::copy(CurrentKeyboardState, &CurrentKeyboardState[SDL_NUM_SCANCODES], KeyboardState.begin());
			::input_state_last = ::input_state;

			auto elapsed_ms = (SDL_GetTicks() - initial_ms);
			if (elapsed_ms < ms_frame) SDL_Delay(ms_frame - elapsed_ms);
		}

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	finalize_m3();
	return 0;
}
