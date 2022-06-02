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

struct rect {
    int x{ 0 };
    int y{ 0 };
    int w{ 0 };
    int h{ 0 };

    inline int left() const { return x; }
    inline int right() const { return x + w; }
    inline int top() const { return y; }
    inline int bottom() const { return y + h; }
};

rect ball{
    (screen_width - ball_width) / 2,
    (screen_height - ball_height) / 2,
    ball_width,
    ball_height
};

rect player_a{
    10,
    0,
    10,
    100
};

bool is_hit(const rect &a, const rect &b) {
    return (a.left() < b.right())
        && (a.top() < b.bottom())
        && (a.right() > b.left())
        && (a.bottom() > b.top());
}

} // namespace

// start
void WASM_EXPORT start()
{

}

// update
int WASM_EXPORT update()
{
    ball.x += x_speed;
    ball.y += y_speed;
    if (ball.x < 0) {
        ball.x = 0;
        x_speed *= -1;

    } else if (ball.x > screen_width) {
        ball.x = screen_width;
        x_speed *= -1;
    }
    if (ball.y < 0) {
        ball.y = 0;
        y_speed *= -1;

    } else if (ball.y > screen_height) {
        ball.y = screen_height;
        y_speed *= -1;
    }

    draw_color(0xFF, 0xFF, 0xFF);
    draw_rect(ball.x, ball.y, ball.w, ball.h);

    if ((x_speed < 0) && is_hit(ball, player_a)) {
        draw_color(0xFF, 0, 0);
        x_speed *= -1;
    }
    draw_rect(player_a.x, player_a.y, player_a.w, player_a.h);
    return 0;
}

} // extern "C" 
