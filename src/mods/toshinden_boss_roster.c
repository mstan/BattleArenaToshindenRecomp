#include "cpu_state.h"
#include "mod_plugins.h"

#include <stdint.h>

#define TOSHINDEN_SELECT_HELPER        0x8017A0D4u
#define TOSHINDEN_SELECT_SCENE         0x801CA584u
#define TOSHINDEN_PLAYER_1             0x801BC114u
#define TOSHINDEN_PLAYER_2             0x801BC1E8u

#define TOSHINDEN_PLAYER_CHAR_OFFSET   0x06u
#define TOSHINDEN_PLAYER_FLAGS_OFFSET  0x40u
#define TOSHINDEN_PLAYER_PAIR_OFFSET   0x34u

#define TOSHINDEN_CHAR_NORMAL_MAX      7
#define TOSHINDEN_CHAR_GAIA            8
#define TOSHINDEN_CHAR_SHO             9

#define TOSHINDEN_INPUT_CONFIRM        0x00F0u
#define TOSHINDEN_INPUT_UP             0x1000u
#define TOSHINDEN_INPUT_DOWN           0x4000u
#define TOSHINDEN_INPUT_NEXT           0x2000u
#define TOSHINDEN_INPUT_PREV           0x8000u
#define TOSHINDEN_INPUT_PAGE           (TOSHINDEN_INPUT_UP | TOSHINDEN_INPUT_DOWN)
#define TOSHINDEN_INPUT_LR             (TOSHINDEN_INPUT_NEXT | TOSHINDEN_INPUT_PREV)
#define TOSHINDEN_INPUT_DIRECTIONAL    (TOSHINDEN_INPUT_PAGE | TOSHINDEN_INPUT_LR)

static int s_boss_roster_enabled;
static int16_t s_previous_normal_char[2] = { 0, 1 };

static int toshinden_player_index(uint32_t player) {
    if (player == TOSHINDEN_PLAYER_1)
        return 0;
    if (player == TOSHINDEN_PLAYER_2)
        return 1;
    return -1;
}

static uint32_t toshinden_other_player(uint32_t player) {
    if (player == TOSHINDEN_PLAYER_1)
        return TOSHINDEN_PLAYER_2;
    if (player == TOSHINDEN_PLAYER_2)
        return TOSHINDEN_PLAYER_1;
    return 0;
}

static int toshinden_is_normal_char(int32_t char_id) {
    return char_id >= 0 && char_id <= TOSHINDEN_CHAR_NORMAL_MAX;
}

static int toshinden_is_boss_char(int32_t char_id) {
    return char_id == TOSHINDEN_CHAR_GAIA || char_id == TOSHINDEN_CHAR_SHO;
}

static void toshinden_set_char(uint32_t player, int16_t char_id) {
    psx_mod_write_half(player + TOSHINDEN_PLAYER_CHAR_OFFSET, (uint16_t)char_id);
}

static void toshinden_set_pair_flag_for_match(uint32_t player, int16_t char_id) {
    uint32_t pair = psx_mod_read_word(player + TOSHINDEN_PLAYER_PAIR_OFFSET);
    uint32_t expected_pair = toshinden_other_player(player);
    uint16_t flags;

    if (pair != expected_pair)
        return;

    if ((int16_t)psx_mod_read_half(pair + TOSHINDEN_PLAYER_CHAR_OFFSET) != char_id) {
        psx_mod_write_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET, 0);
        return;
    }

    flags = psx_mod_read_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET);
    if ((flags & 2u) == 0u) {
        uint16_t pair_flags = psx_mod_read_half(pair + TOSHINDEN_PLAYER_FLAGS_OFFSET);
        flags = (uint16_t)((pair_flags ^ 1u) & 1u);
        psx_mod_write_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET, flags);
    }
}

static void toshinden_suppress_directional_input(CPUState *cpu) {
    cpu->gpr[6] &= ~TOSHINDEN_INPUT_DIRECTIONAL;
}

static void toshinden_enter_boss_page(CPUState *cpu, uint32_t player,
                                      int index, int16_t current_char) {
    if (toshinden_is_normal_char(current_char))
        s_previous_normal_char[index] = current_char;

    toshinden_set_char(player, TOSHINDEN_CHAR_GAIA);
    psx_mod_write_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET, 0);
    toshinden_suppress_directional_input(cpu);
}

static void toshinden_leave_boss_page(CPUState *cpu, uint32_t player,
                                      int index) {
    int16_t restore = s_previous_normal_char[index];

    if (!toshinden_is_normal_char(restore))
        restore = index == 0 ? 0 : 1;

    toshinden_set_char(player, restore);
    psx_mod_write_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET, 0);
    toshinden_suppress_directional_input(cpu);
}

static void toshinden_toggle_boss(CPUState *cpu, uint32_t player,
                                  int16_t current_char) {
    int16_t next = current_char == TOSHINDEN_CHAR_GAIA ?
        TOSHINDEN_CHAR_SHO : TOSHINDEN_CHAR_GAIA;

    toshinden_set_char(player, next);
    toshinden_set_pair_flag_for_match(player, next);
    toshinden_suppress_directional_input(cpu);
}

static void toshinden_boss_select_helper_entry(CPUState *cpu,
                                               uint32_t address) {
    uint32_t scene;
    uint32_t player;
    uint16_t input;
    int index;
    int16_t current_char;

    if (!s_boss_roster_enabled || address != TOSHINDEN_SELECT_HELPER)
        return;

    scene = cpu->gpr[4];
    player = cpu->gpr[5];
    if (scene != TOSHINDEN_SELECT_SCENE)
        return;

    index = toshinden_player_index(player);
    if (index < 0)
        return;

    current_char = (int16_t)psx_mod_read_half(player + TOSHINDEN_PLAYER_CHAR_OFFSET);
    if (toshinden_is_normal_char(current_char))
        s_previous_normal_char[index] = current_char;

    input = (uint16_t)cpu->gpr[6];

    if ((input & TOSHINDEN_INPUT_CONFIRM) != 0u) {
        if (toshinden_is_boss_char(current_char))
            toshinden_set_pair_flag_for_match(player, current_char);
        return;
    }

    if (!toshinden_is_boss_char(current_char)) {
        if ((input & TOSHINDEN_INPUT_PAGE) != 0u)
            toshinden_enter_boss_page(cpu, player, index, current_char);
        return;
    }

    if ((input & TOSHINDEN_INPUT_PAGE) != 0u) {
        toshinden_leave_boss_page(cpu, player, index);
        return;
    }

    if ((input & TOSHINDEN_INPUT_LR) != 0u)
        toshinden_toggle_boss(cpu, player, current_char);
}

static void toshinden_boss_roster_activate(void) {
    s_boss_roster_enabled = 1;
}

PSX_MOD_CONSTRUCTOR(toshinden_register_boss_roster_plugins) {
    (void)psx_mod_register_activation_plugin(
        "toshinden.boss-roster.enable", toshinden_boss_roster_activate);
    (void)psx_mod_register_function_entry_plugin(
        "toshinden.boss-roster.select-helper", TOSHINDEN_SELECT_HELPER,
        toshinden_boss_select_helper_entry);
}
