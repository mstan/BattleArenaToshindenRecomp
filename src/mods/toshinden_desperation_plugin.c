#include "mod_plugins.h"

#include <stdint.h>

#define TOSHINDEN_DESPERATION_PREV_ADDR      0x80186C40u
#define TOSHINDEN_DESPERATION_GATE_ADDR      0x80186C48u
#define TOSHINDEN_DESPERATION_NEXT_ADDR      0x80186C4Cu

#define TOSHINDEN_DESPERATION_PREV_EXPECTED  0x8F830280u
#define TOSHINDEN_DESPERATION_GATE_EXPECTED  0x2C630050u
#define TOSHINDEN_DESPERATION_GATE_PATCHED   0x34030001u
#define TOSHINDEN_DESPERATION_NEXT_EXPECTED  0x14600003u

static int s_toshinden_desperation_enabled;

static void toshinden_desperation_apply(void) {
    uint32_t current;

    if (!s_toshinden_desperation_enabled)
        return;

    current = psx_mod_read_word(TOSHINDEN_DESPERATION_GATE_ADDR);
    if (current != TOSHINDEN_DESPERATION_GATE_EXPECTED &&
        current != TOSHINDEN_DESPERATION_GATE_PATCHED)
        return;
    if (psx_mod_read_word(TOSHINDEN_DESPERATION_PREV_ADDR) !=
        TOSHINDEN_DESPERATION_PREV_EXPECTED)
        return;
    if (psx_mod_read_word(TOSHINDEN_DESPERATION_NEXT_ADDR) !=
        TOSHINDEN_DESPERATION_NEXT_EXPECTED)
        return;

    /*
     * func_80186BFC is the type-4 command matcher used by the character
     * command-list dispatcher. The stock gate is:
     *   lw    v1, 0x280(gp)     ; current player health ratio
     *   sltiu v1, v1, 0x50      ; require low health
     *   bnez  v1, success
     *
     * Replace only the compare with `ori v1, zero, 1`. Command input, player
     * indexing, damage, KO, health, max HP, and gauges continue through the
     * stock code paths.
     */
    psx_mod_write_code_word(TOSHINDEN_DESPERATION_GATE_ADDR,
                            TOSHINDEN_DESPERATION_GATE_PATCHED);
}

static void toshinden_desperation_activate(void) {
    s_toshinden_desperation_enabled = 1;
    toshinden_desperation_apply();
}

static void toshinden_desperation_vblank(void) {
    toshinden_desperation_apply();
}

PSX_MOD_CONSTRUCTOR(toshinden_register_desperation_plugin) {
    (void)psx_mod_register_activation_plugin(
        "toshinden.desperation.any-health", toshinden_desperation_activate);
    (void)psx_mod_register_vblank_plugin(
        "toshinden.desperation.any-health", toshinden_desperation_vblank);
}