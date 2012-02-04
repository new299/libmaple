#include "wirish.h"
#include "OLED.h"

#define WIDTH 128/8
#define HEIGHT 128/8

static const uint8 *tiles[WIDTH*HEIGHT];

void tile_draw(uint8 x, uint8 y, const uint8 *data) {
    oled_draw_rect(x*8, y*8, 8, 8, data);
}

void tile_set(uint8 x, uint8 y, const uint8 *data) {
    if (tiles[y*WIDTH+x] != data) {
        /*
        Serial1.print("Tile at "); Serial1.print(x, 10);
        Serial1.print(","); Serial1.print(y, 10); Serial1.println(" was not cached");
        */
        tiles[y*WIDTH+x] = data;
        tile_draw(x, y, data);
    }
}
