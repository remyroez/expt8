#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// wasm
#define WASM_EXPORT __attribute__((used)) __attribute__((visibility ("default")))

extern "C" {

// API
extern int draw_color(int, int, int);
extern int draw_rect(int, int, int, int);

namespace {

constexpr int screen_width = 256;
constexpr int screen_height = 240;

constexpr int ball_width = 10;
constexpr int ball_height = 10;

int x_speed = 5;
int y_speed = 5;

int x = (screen_width - ball_width) / 2;
int y = (screen_height - ball_height) / 2;

} // namespace

// start
void WASM_EXPORT start()
{

}

// update
int WASM_EXPORT update()
{
    x += x_speed;
    y += y_speed;
    if (x < 0) {
        x = 0;
        x_speed *= -1;

    } else if (x > screen_width) {
        x = screen_width;
        x_speed *= -1;
    }
    if (y < 0) {
        y = 0;
        y_speed *= -1;

    } else if (y > screen_height) {
        y = screen_height;
        y_speed *= -1;
    }

    draw_color(0xFF, 0xFF, 0xFF);
    draw_rect(x, y, ball_width, ball_height);
    return 0;
}

} // extern "C" 
