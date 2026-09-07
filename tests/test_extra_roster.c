#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_state.h"
#include "mod_plugins.h"

#define RAM_BASE 0x801BC000u
#define RAM_SIZE 0x400u

static uint8_t ram[RAM_SIZE];
static int failures;

static size_t ram_offset(uint32_t address) {
    if (address < RAM_BASE || address >= RAM_BASE + RAM_SIZE) {
        printf("FAIL: address out of test RAM 0x%08x\n", address);
        exit(2);
    }
    return (size_t)(address - RAM_BASE);
}

uint8_t psx_mod_read_byte(uint32_t address) { return ram[ram_offset(address)]; }
void psx_mod_write_byte(uint32_t address, uint8_t value) { ram[ram_offset(address)] = value; }

uint16_t psx_mod_read_half(uint32_t address) {
    size_t i = ram_offset(address);
    return (uint16_t)(ram[i] | ((uint16_t)ram[i + 1u] << 8));
}

void psx_mod_write_half(uint32_t address, uint16_t value) {
    size_t i = ram_offset(address);
    ram[i] = (uint8_t)value;
    ram[i + 1u] = (uint8_t)(value >> 8);
}

uint32_t psx_mod_read_word(uint32_t address) {
    size_t i = ram_offset(address);
    return (uint32_t)ram[i] |
           ((uint32_t)ram[i + 1u] << 8) |
           ((uint32_t)ram[i + 2u] << 16) |
           ((uint32_t)ram[i + 3u] << 24);
}

void psx_mod_write_word(uint32_t address, uint32_t value) {
    size_t i = ram_offset(address);
    ram[i] = (uint8_t)value;
    ram[i + 1u] = (uint8_t)(value >> 8);
    ram[i + 2u] = (uint8_t)(value >> 16);
    ram[i + 3u] = (uint8_t)(value >> 24);
}

void psx_mod_write_code_word(uint32_t address, uint32_t value) { (void)address; (void)value; }
uint32_t psx_mod_alloc_guest_memory(uint32_t size, uint32_t alignment) { (void)size; (void)alignment; return 0; }
uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t size, uint32_t alignment) { (void)size; (void)alignment; return 0; }
int psx_mod_game_started(void) { return 1; }
int32_t psx_mod_widescreen_x_margin(void) { return 0; }
uint32_t psx_mod_display_width(void) { return 320; }
uint32_t psx_mod_display_height(void) { return 240; }
int psx_mod_option_value(const char *package_id, const char *feature_id,
                         const char *option_id, char *out, uint32_t out_size) {
    (void)package_id; (void)feature_id; (void)option_id;
    if (out != NULL && out_size != 0u) out[0] = '\0';
    return 0;
}
int psx_mod_current_resource_path(const char *resource_id, char *out, uint32_t out_size) {
    (void)resource_id;
    if (out != NULL && out_size != 0u) out[0] = '\0';
    return 0;
}
int psx_mod_set_fixed_display_aspect(uint32_t numerator, uint32_t denominator) { (void)numerator; (void)denominator; return 1; }
int psx_mod_set_adaptive_display_aspect(uint32_t max_numerator, uint32_t denominator) { (void)max_numerator; (void)denominator; return 1; }
int psx_mod_set_native_vblank_rate(uint32_t frames_per_second) { (void)frames_per_second; return 1; }
int psx_mod_set_frame_interpolation(uint32_t frames_per_second) { (void)frames_per_second; return 1; }
int psx_mod_set_frame_interpolation_blend(uint32_t blend_mode) { (void)blend_mode; return 1; }
int psx_mod_set_auto_skip_fmv(int enabled) { (void)enabled; return 1; }
int psx_mod_set_bezel_artwork(const char *path) { (void)path; return 1; }
int psx_mod_set_load_acceleration(uint32_t wall_clock_multiplier, uint32_t read_speed_multiplier) { (void)wall_clock_multiplier; (void)read_speed_multiplier; return 1; }
int psx_mod_set_disc_speed(uint32_t divisor, uint32_t seek_divisor) { (void)divisor; (void)seek_divisor; return 1; }

int psx_mod_register_activation_plugin(const char *id, PSXModActivationCallback callback) {
    (void)id;
    callback();
    return 1;
}
int psx_mod_register_vblank_plugin(const char *id, PSXModVBlankCallback callback) { (void)id; (void)callback; return 1; }
int psx_mod_register_function_entry_plugin(const char *id, uint32_t address, PSXModFunctionEntryCallback callback) { (void)id; (void)address; (void)callback; return 1; }
void psx_mod_function_entry(struct CPUState *cpu, uint32_t address) { (void)cpu; (void)address; }

#include "../src/mods/toshinden_boss_roster.c"

#define CHECK_U16(name, got, want) do { \
    uint16_t got_value = (uint16_t)(got); \
    uint16_t want_value = (uint16_t)(want); \
    if (got_value != want_value) { \
        printf("FAIL: %s got %u want %u\n", name, got_value, want_value); \
        failures++; \
    } else { \
        printf("ok:   %s\n", name); \
    } \
} while (0)

static void reset_select_state(void) {
    memset(ram, 0, sizeof(ram));
    s_boss_roster_enabled = 1;
    psx_mod_write_word(TOSHINDEN_PLAYER_1 + TOSHINDEN_PLAYER_PAIR_OFFSET,
                       TOSHINDEN_PLAYER_2);
    psx_mod_write_word(TOSHINDEN_PLAYER_2 + TOSHINDEN_PLAYER_PAIR_OFFSET,
                       TOSHINDEN_PLAYER_1);
}

static void set_char(uint32_t player, uint16_t ch) {
    psx_mod_write_half(player + TOSHINDEN_PLAYER_CHAR_OFFSET, ch);
}

static uint16_t char_of(uint32_t player) {
    return psx_mod_read_half(player + TOSHINDEN_PLAYER_CHAR_OFFSET);
}

static void set_flags(uint32_t player, uint16_t flags) {
    psx_mod_write_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET, flags);
}

static uint16_t flags_of(uint32_t player) {
    return psx_mod_read_half(player + TOSHINDEN_PLAYER_FLAGS_OFFSET);
}

static void invoke_select_helper(uint32_t player, uint32_t input) {
    CPUState cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.gpr[4] = TOSHINDEN_SELECT_SCENE;
    cpu.gpr[5] = player;
    cpu.gpr[6] = input;
    toshinden_boss_select_helper_entry(&cpu, TOSHINDEN_SELECT_HELPER);
}

static void test_explicit_select_survives_different_opponent_for_both_players(void) {
    reset_select_state();
    set_char(TOSHINDEN_PLAYER_1, TOSHINDEN_CHAR_GAIA);
    set_char(TOSHINDEN_PLAYER_2, 4);
    set_flags(TOSHINDEN_PLAYER_1,
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
    invoke_select_helper(TOSHINDEN_PLAYER_1, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("P1 explicit alternate survives different opponent confirm",
              flags_of(TOSHINDEN_PLAYER_1),
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);

    reset_select_state();
    set_char(TOSHINDEN_PLAYER_2, TOSHINDEN_CHAR_SHO);
    set_char(TOSHINDEN_PLAYER_1, 3);
    set_flags(TOSHINDEN_PLAYER_2, TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT);
    invoke_select_helper(TOSHINDEN_PLAYER_2, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("P2 explicit default-color select survives different opponent confirm",
              flags_of(TOSHINDEN_PLAYER_2), TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT);
}

static void test_explicit_select_survives_mirror_and_single_player(void) {
    reset_select_state();
    set_char(TOSHINDEN_PLAYER_1, TOSHINDEN_CHAR_SHO);
    set_char(TOSHINDEN_PLAYER_2, TOSHINDEN_CHAR_SHO);
    set_flags(TOSHINDEN_PLAYER_1,
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
    set_flags(TOSHINDEN_PLAYER_2, 0);
    invoke_select_helper(TOSHINDEN_PLAYER_1, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("explicit alternate is not auto-overridden in mirror confirm",
              flags_of(TOSHINDEN_PLAYER_1),
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);

    reset_select_state();
    psx_mod_write_word(TOSHINDEN_PLAYER_1 + TOSHINDEN_PLAYER_PAIR_OFFSET, 0);
    set_char(TOSHINDEN_PLAYER_1, TOSHINDEN_CHAR_GAIA);
    set_flags(TOSHINDEN_PLAYER_1,
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
    invoke_select_helper(TOSHINDEN_PLAYER_1, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("explicit alternate survives no-pair single-player confirm",
              flags_of(TOSHINDEN_PLAYER_1),
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
}

static void test_unconfirmed_auto_color_policy(void) {
    reset_select_state();
    set_char(TOSHINDEN_PLAYER_1, TOSHINDEN_CHAR_GAIA);
    set_char(TOSHINDEN_PLAYER_2, 6);
    set_flags(TOSHINDEN_PLAYER_1, TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
    invoke_select_helper(TOSHINDEN_PLAYER_1, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("unconfirmed P1 extra clears auto color against different opponent",
              flags_of(TOSHINDEN_PLAYER_1), 0);

    reset_select_state();
    set_char(TOSHINDEN_PLAYER_1, TOSHINDEN_CHAR_GAIA);
    set_char(TOSHINDEN_PLAYER_2, TOSHINDEN_CHAR_GAIA);
    set_flags(TOSHINDEN_PLAYER_1, 0);
    set_flags(TOSHINDEN_PLAYER_2, 0);
    invoke_select_helper(TOSHINDEN_PLAYER_1, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("unconfirmed P1 mirror auto-picks opposite color",
              flags_of(TOSHINDEN_PLAYER_1), TOSHINDEN_PLAYER_FLAG_ALT_COLOR);

    reset_select_state();
    set_char(TOSHINDEN_PLAYER_1, TOSHINDEN_CHAR_SHO);
    set_char(TOSHINDEN_PLAYER_2, TOSHINDEN_CHAR_SHO);
    set_flags(TOSHINDEN_PLAYER_1, 0);
    set_flags(TOSHINDEN_PLAYER_2, 0);
    invoke_select_helper(TOSHINDEN_PLAYER_2, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("unconfirmed P2 mirror auto-picks opposite color",
              flags_of(TOSHINDEN_PLAYER_2), TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
}

static void test_regular_roster_confirm_does_not_touch_native_select_flags(void) {
    reset_select_state();
    set_char(TOSHINDEN_PLAYER_1, 3);
    set_char(TOSHINDEN_PLAYER_2, 4);
    set_flags(TOSHINDEN_PLAYER_1,
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
    invoke_select_helper(TOSHINDEN_PLAYER_1, TOSHINDEN_INPUT_CONFIRM);
    CHECK_U16("regular roster confirm leaves character unchanged",
              char_of(TOSHINDEN_PLAYER_1), 3);
    CHECK_U16("regular roster confirm leaves native select flags unchanged",
              flags_of(TOSHINDEN_PLAYER_1),
              TOSHINDEN_PLAYER_FLAG_ALT_EXPLICIT | TOSHINDEN_PLAYER_FLAG_ALT_COLOR);
}

int main(void) {
    toshinden_register_boss_roster_plugins();
    test_explicit_select_survives_different_opponent_for_both_players();
    test_explicit_select_survives_mirror_and_single_player();
    test_unconfirmed_auto_color_policy();
    test_regular_roster_confirm_does_not_touch_native_select_flags();

    if (failures != 0) {
        printf("%d failures\n", failures);
        return 1;
    }

    puts("all extra roster checks passed");
    return 0;
}
