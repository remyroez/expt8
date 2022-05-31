
#include <stdio.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <SDL.h>

#include <wasm3.h>
#include <m3_env.h>

namespace {

constexpr int logical_width = 256;
constexpr int logical_height = 244;
constexpr int default_scale = 2;
constexpr int default_width = (logical_width * default_scale);
constexpr int default_height = (logical_height * default_scale);
constexpr Uint32 fps = 60;
constexpr Uint32 ms_frame = (1000U / fps);
constexpr uint32_t memory_size = (1024 * 64);

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

IM3Environment environment = nullptr;
IM3Runtime runtime = nullptr;
IM3Module module = nullptr;

IM3Function test = nullptr;
IM3Function test_memcpy = nullptr;
IM3Function test_counter_get = nullptr;
IM3Function test_counter_inc = nullptr;
IM3Function test_counter_add = nullptr;

void finalize_m3() {
	test_counter_get = nullptr;
	test_counter_inc = nullptr;
	test_counter_add = nullptr;
	module = nullptr;
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

bool setup_wasm(const char *file_path) {
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
			m3_LinkRawFunction(module, "*", "sum", "i(ii)", wasm_sum);
			m3_LinkRawFunction(module, "*", "ext_memcpy", "*(**i)", wasm_ext_memcpy);
			m3_FindFunction(&test, runtime, "test");
			m3_FindFunction(&test_memcpy, runtime, "test_memcpy");
			m3_FindFunction(&test_counter_get, runtime, "test_counter_get");
			m3_FindFunction(&test_counter_inc, runtime, "test_counter_inc");
			m3_FindFunction(&test_counter_add, runtime, "test_counter_add");

			{
				m3_CallV(test, 20, 10);
				int result = 0;
				m3_GetResultsV(test, &result);
				SDL_Log("test -> %d", result);
			}

			{
				m3_CallV(test_memcpy);
				int64_t result = 0;
				m3_GetResultsV(test_memcpy, &result);
				SDL_Log("test_memcpy -> %llx", result);
			}

			{
				m3_CallV(test_counter_get);
				int result = 0;
				m3_GetResultsV(test_counter_get, &result);
				SDL_Log("test_counter_get -> %d", result);
			}

			{
				m3_CallV(test_counter_inc);
				m3_CallV(test_counter_get);
				int result = 0;
				m3_GetResultsV(test_counter_get, &result);
				SDL_Log("test_counter_inc -> test_counter_get -> %d", result);
			}

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
	if (!setup_wasm("wasm/test_prog.wasm")) {
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

	finalize_m3();
	return 0;
}
