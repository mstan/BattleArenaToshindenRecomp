#include "cpu_state.h"
#include "gpu.h"
#include "mod_plugins.h"

#include <stdint.h>

#define TOSHINDEN_ADD_PRIM              0x80196874u

#define TOSHINDEN_PANORAMA_SUBMIT       0x80183BE8u

#define TOSHINDEN_PANORAMA_H            128
#define TOSHINDEN_VIEWPORT_W            640
#define TOSHINDEN_PANORAMA_PERIOD       512
#define TOSHINDEN_PANORAMA_STRIP_W      256

/*
 * Framework-owned renderer hook, added with the shared GPU work:
 * mark this clipped SPRT as a tiled source strip. The renderer sees the actual
 * GP0 state, including current tpage, and reconstructs the original 256-wide
 * strip from the clipped packet before drawing repeated copies at native texel
 * density in the wide surface.
 */
static int s_toshinden_backdrop_enabled;

static int toshinden_is_panorama_submit(uint32_t caller_ra) {
    return caller_ra == TOSHINDEN_PANORAMA_SUBMIT;
}

static int toshinden_is_main_ram_prim(uint32_t prim) {
    return prim >= 0x80000000u && prim < 0x80200000u;
}

static void toshinden_backdrop_prim_entry(CPUState *cpu, uint32_t address) {
    uint32_t prim;
    uint32_t caller_ra;
    uint8_t op;
    int16_t x;
    int16_t y;
    int32_t w;
    int32_t h;
    uint32_t display_width;

    if (!s_toshinden_backdrop_enabled || address != TOSHINDEN_ADD_PRIM)
        return;

    /*
     * 0x80196874 is the addPrim helper. At its entry:
     *   a0 = OT node, a1 = primitive tag address, ra = submit site.
     * Keep this hook disjoint from adaptive HUD handling by accepting only the
     * traced 0x80183A34 panorama strip submit and the observed row shape.
     */
    prim = cpu->gpr[5];
    caller_ra = cpu->gpr[31];
    if (!toshinden_is_main_ram_prim(prim) ||
        !toshinden_is_panorama_submit(caller_ra))
        return;

    op = psx_mod_read_byte(prim + 7u);
    if (op != 0x65u)
        return;

    x = (int16_t)psx_mod_read_half(prim + 8u);
    y = (int16_t)psx_mod_read_half(prim + 10u);
    w = (int32_t)(psx_mod_read_half(prim + 16u) & 0x03FFu);
    h = (int32_t)(psx_mod_read_half(prim + 18u) & 0x01FFu);

    if (y <= -TOSHINDEN_PANORAMA_H || y >= 240 ||
        h != TOSHINDEN_PANORAMA_H ||
        w <= 0 || w > 256 ||
        x < 0 || x >= TOSHINDEN_VIEWPORT_W ||
        (int32_t)x + w > TOSHINDEN_VIEWPORT_W)
        return;

    display_width = psx_mod_display_width();
    if (display_width != TOSHINDEN_VIEWPORT_W)
        return;

    /*
     * 0x80183A34 advances four 256-wide source slots modulo 1024, then clips
     * them to the visible 640-pixel viewport before calling addPrim from
     * 0x80183BE8. func_80183C34 initializes all four strips with u=0, one CLUT,
     * and one tpage stream; the slots at +0x21A8/+0x21D0 use v=0 and
     * +0x21BC/+0x21E4 use v=128. The texture therefore repeats visually every
     * two strips, or 512 pixels, despite the 1024-coordinate phase.
     *
     * gpu_ws_tag_tiled_strip restores x -= u, u = 0, w = 256 at render time.
     * That recovers clipped edge strips such as the traced x=0/u=0x3B/w=197
     * packet before repeating the 512-pixel source period into the margins.
     * Duplicate modulo-512 strips can overdraw the same pixels, but opcode 0x65
     * is opaque and the paired rows are identical, so the overlap is benign.
     */
    gpu_ws_tag_tiled_strip(prim, (int32_t)display_width / 2,
                           TOSHINDEN_PANORAMA_PERIOD,
                           TOSHINDEN_PANORAMA_STRIP_W);
}

static void toshinden_widescreen_backdrop_activate(void) {
    s_toshinden_backdrop_enabled = 1;
}

PSX_MOD_CONSTRUCTOR(toshinden_register_widescreen_backdrop_plugin) {
    (void)psx_mod_register_activation_plugin(
        "toshinden.widescreen.backdrop-preserve",
        toshinden_widescreen_backdrop_activate);
    (void)psx_mod_register_function_entry_plugin(
        "toshinden.widescreen.backdrop-prim", TOSHINDEN_ADD_PRIM,
        toshinden_backdrop_prim_entry);
}
