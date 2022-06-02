#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// wasm
#define WASM_EXPORT __attribute__((used)) __attribute__((visibility ("default")))

extern "C" {

// API
extern int draw_color(int, int, int, int);
extern int draw_rect(int, int, int, int);

// define
#define WIDTH 256
#define HEIGHT 244

// start
void WASM_EXPORT start()
{

}

// update
int WASM_EXPORT update()
{
    draw_color(0xFF, 0, 0, 0);
    draw_rect(10, 10, 110, 110);
    return 0;
}

} // extern "C" 
