#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

extern int sum(int, int);
extern int ext_memcpy(void*, const void*, size_t);

extern int draw_color(int, int, int, int);
extern int draw_rect(int, int, int, int);

#define WIDTH 256
#define HEIGHT 244

int32_t counter = 0;

#define WASM_EXPORT __attribute__((used)) __attribute__((visibility ("default")))

void WASM_EXPORT start()
{

}

int WASM_EXPORT update()
{
#if 1
    int x = 0, y = 0, w = 0, h = 0;
    for (int i = 0; i < 100; ++i) {
        draw_color(rand() % 0xFF, rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);
        x = rand() % WIDTH;
        y = rand() % HEIGHT;
        w = rand() % (WIDTH - x);
        h = rand() % (HEIGHT - y);
        draw_rect(x, y , w, h);
    }
#endif
    return 123;
}

int WASM_EXPORT test(int32_t arg1, int32_t arg2)
{
    int x = arg1 + arg2;
    int y = arg1 - arg2;
    return sum(x, y) / 2;
}

int64_t WASM_EXPORT test_memcpy(void)
{
    int64_t x = 0;
    int32_t low = 0x01234567;
    int32_t high = 0x89abcdef;
    ext_memcpy(&x, &low, 4);
    ext_memcpy(((uint8_t*)&x) + 4, &high, 4);
    return x;
}

int32_t WASM_EXPORT test_counter_get()
{
    return counter;
}

void WASM_EXPORT test_counter_inc()
{
    ++counter;
}

void WASM_EXPORT test_counter_add(int32_t inc_value)
{
    counter += inc_value;
}
