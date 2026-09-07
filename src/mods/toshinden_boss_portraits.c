#include "gpu.h"
#include "toshinden_boss_portraits.h"

#include <string.h>

#define TOSHINDEN_BOSS_PORTRAIT_W 128u
#define TOSHINDEN_BOSS_PORTRAIT_H 128u
#define TOSHINDEN_BOSS_PORTRAIT_WORDS \
    (TOSHINDEN_BOSS_PORTRAIT_W * TOSHINDEN_BOSS_PORTRAIT_H)

/* These native atlas rows contain alternate portraits. The UI command stream
 * borrows them only after native drawing, then uploads these pixels back.
 * Re-snapshot every frame; never persist mod artwork in the native atlas. */
static uint16_t s_gaia_restore[TOSHINDEN_BOSS_PORTRAIT_WORDS];
static uint16_t s_sho_restore[TOSHINDEN_BOSS_PORTRAIT_WORDS];
static int s_restore_valid;

static void snapshot_portrait_region(int x, int y, uint16_t *out) {
    const uint16_t *vram = gpu_get_vram();

    for (uint32_t row = 0; row < TOSHINDEN_BOSS_PORTRAIT_H; row++) {
        memcpy(out + row * TOSHINDEN_BOSS_PORTRAIT_W,
               vram + ((uint32_t)y + row) * 1024u + (uint32_t)x,
               TOSHINDEN_BOSS_PORTRAIT_W * sizeof(uint16_t));
    }
}

void toshinden_boss_portraits_snapshot_native(int gaia_x, int gaia_y,
                                              int sho_x, int sho_y) {
    snapshot_portrait_region(gaia_x, gaia_y, s_gaia_restore);
    snapshot_portrait_region(sho_x, sho_y, s_sho_restore);
    s_restore_valid = 1;
}

int toshinden_boss_portraits_restore_valid(void) {
    return s_restore_valid;
}

const uint16_t *toshinden_boss_portrait_pixels(int char_id) {
    switch (char_id) {
    case 8:
        return toshinden_gaia_portrait;
    case 9:
        return toshinden_sho_portrait;
    default:
        return 0;
    }
}

const uint16_t *toshinden_boss_portrait_restore_pixels(int char_id) {
    if (!s_restore_valid)
        return 0;

    switch (char_id) {
    case 8:
        return s_gaia_restore;
    case 9:
        return s_sho_restore;
    default:
        return 0;
    }
}
