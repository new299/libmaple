#include "wirish.h"
#include "OLED.h"

void tile_draw(uint8 x, uint8 y, uint8 *data) {
    OLED_draw_rect(x*8, y*8, 8, 8, data);
}
