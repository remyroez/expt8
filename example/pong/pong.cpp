#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// wasm
#define WASM_EXPORT __attribute__((used)) __attribute__((visibility ("default")))

extern "C" {

// API
extern int draw_color(int, int, int);
extern int draw_rect(int, int, int, int);
extern int input(int);
extern int press(int);

} // extern "C" 

enum {
    input_right,
    input_left,
    input_down,
    input_up,
    input_a,
    input_b,
    input_start,
    input_select,
};

namespace {

constexpr int screen_width = 256;
constexpr int screen_height = 240;

constexpr int ball_width = 8;
constexpr int ball_height = 8;

int x_speed = 4;
int y_speed = 4;

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

    inline void left(int value) { x = value; }
    inline void right(int value) { x = value - w; }
    inline void top(int value) { y = value; }
    inline void bottom(int value) { y = value - h; }
};

rect ball{
    (screen_width - ball_width) / 2,
    (screen_height - ball_height) / 2,
    ball_width,
    ball_height
};

rect player_a{
    8,
    0,
    8,
    64
};

rect player_b{
    screen_width - 16,
    0,
    8,
    64
};

bool is_hit(const rect &a, const rect &b) {
    return (a.left() < b.right())
        && (a.top() < b.bottom())
        && (a.right() > b.left())
        && (a.bottom() > b.top());
}

bool is_hit2(const rect &a, const rect &b) {
    return (a.left() < b.right())
        && (a.top() < b.bottom())
        && (a.right() > b.left())
        && (a.bottom() > b.top());
}

} // namespace

// start
extern "C" void WASM_EXPORT start()
{

}

// update
extern "C" int WASM_EXPORT update()
{
    ball.x += x_speed;
    ball.y += y_speed;
    if (ball.left() < 0) {
        ball.left(0);
        x_speed *= -1;

    } else if (ball.right() > screen_width) {
        ball.right(screen_width);
        x_speed *= -1;
    }
    if (ball.top() < 0) {
        ball.top(0);
        y_speed *= -1;

    } else if (ball.bottom() > screen_height) {
        ball.bottom(screen_height);
        y_speed *= -1;
    }

    draw_color(0xFF, 0xFF, 0xFF);
    draw_rect(ball.x, ball.y, ball.w, ball.h);

    if (input(input_up)) {
        player_a.y -= 4;
        if (player_a.top() < 0) {
            player_a.top(0);
        }
    }
    if (input(input_down)) {
        player_a.y += 4;
        if (player_a.bottom() > screen_height) {
            player_a.bottom(screen_height);
        }
    }
    //if (input(input_left)) player_a.x -= 4;
    //if (input(input_right)) player_a.x += 4;
    if (press(input_a)) draw_rect(0, screen_height - 8, 8, 8);
    if (press(input_b)) draw_rect(8, screen_height - 8, 8, 8);
    if (press(input_start)) draw_rect(16, screen_height - 8, 8, 8);
    if (press(input_select)) draw_rect(24, screen_height - 8, 8, 8);

    if ((x_speed < 0) && is_hit(ball, player_a)) {
        draw_color(0xFF, 0, 0);
        x_speed *= -1;
    } else {
        draw_color(0xFF, 0xFF, 0);
    }
    draw_rect(player_a.x, player_a.y, player_a.w, player_a.h);
    is_hit2(ball, player_a);
#if 0
    if ((x_speed > 0) && is_hit(ball, player_b)) {
        draw_color(0xFF, 0, 0);
        x_speed *= -1;
    } else {
        draw_color(0, 0xFF, 0xFF);
    }
    draw_rect(player_b.x, player_b.y, player_b.w, player_b.h);
    #endif
    return 0;
}
