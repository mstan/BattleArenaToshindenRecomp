#include "gpu.h"
#include "toshinden_boss_portraits.h"

#include <string.h>

/* Character select uses the original eight indexed portraits at y=0..127,
 * plus Duke/Ellis at y=256..383. Its background/font live at x>=896,y>=256.
 * The unused middle rows below the first portraits hold two mod-owned 16-bit
 * images. This runs only when the boss UI proves a live select screen. */
static void prepare_portrait(int x, const uint16_t *pixels) {
    const uint16_t *vram = gpu_get_vram();
    int changed = 0;
    for (int row = 0; row < 128; ++row) {
        if (memcmp(vram + (128 + row) * 1024 + x,
                   pixels + row * 128, 128 * sizeof(uint16_t)) != 0) {
            changed = 1;
            break;
        }
    }
    if (!changed) return;

    /* Use the GPU upload path so the CPU mirror, OpenGL texture and texture
     * caches agree. Comparing VRAM also makes savestate restores safe. */
    gpu_write_gp0(0xA0000000u);
    gpu_write_gp0((128u << 16) | (uint32_t)x);
    gpu_write_gp0((128u << 16) | 128u);
    for (int i = 0; i < 16384; i += 2)
        gpu_write_gp0((uint32_t)pixels[i] | ((uint32_t)pixels[i + 1] << 16));
}

void toshinden_boss_portraits_prepare(void) {
    prepare_portrait(640, toshinden_gaia_portrait);
    prepare_portrait(768, toshinden_sho_portrait);
}
