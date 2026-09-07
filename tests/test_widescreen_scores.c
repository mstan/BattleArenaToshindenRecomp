#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_state.h"

#define RAM_BASE 0x80000000u
#define RAM_SIZE 0x00200000u
#define FINAL_OT 0x80197C70u
#define OT_HEAD0 0x801F25C0u
#define OT_HEAD1 0x801F4ED0u
#define BATTLE_UI_BASE 0x801BC2BCu
#define BATTLE_UI_STRIDE 0x4B28u
#define SCORE_POOL_LO 0x1E0u
#define SCORE_POOL_HI 0x320u
#define SCORE_GLYPH_STRIDE 0x14u
#define SCORE_DIGITS 8u
#define DISPLAY_W 640u
#define CENTER_X ((int32_t)(DISPLAY_W / 2u))

static uint8_t ram[RAM_SIZE];
static int failures;
static int ws_margin = 160;
static uint32_t display_width = DISPLAY_W;

struct tag_call {
    uint32_t packet;
    int32_t anchor;
};

struct stretch_call {
    uint32_t packet;
    int left_dst;
    int right_dst;
    int left_src;
    int right_src;
};

static struct tag_call tags[256];
static int tag_count;
static struct stretch_call stretches[64];
static int stretch_count;
static int fixed_aspect_calls;
static int adaptive_aspect_calls;
static int activation_regs;
static int entry_regs;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
    else         { printf("ok:   %s\n", msg); } \
} while (0)

static size_t off(uint32_t address) {
    if (address < RAM_BASE || address >= RAM_BASE + RAM_SIZE) {
        printf("FAIL: address out of RAM 0x%08x\n", address);
        exit(2);
    }
    return (size_t)(address - RAM_BASE);
}

uint8_t psx_mod_read_byte(uint32_t address) { return ram[off(address)]; }
void psx_mod_write_byte(uint32_t address, uint8_t value) { ram[off(address)] = value; }
uint16_t psx_mod_read_half(uint32_t address) {
    size_t i = off(address);
    return (uint16_t)(ram[i] | ((uint16_t)ram[i + 1] << 8));
}
void psx_mod_write_half(uint32_t address, uint16_t value) {
    size_t i = off(address);
    ram[i] = (uint8_t)value;
    ram[i + 1] = (uint8_t)(value >> 8);
}
uint32_t psx_mod_read_word(uint32_t address) {
    size_t i = off(address);
    return (uint32_t)ram[i] | ((uint32_t)ram[i + 1] << 8) |
           ((uint32_t)ram[i + 2] << 16) | ((uint32_t)ram[i + 3] << 24);
}
void psx_mod_write_word(uint32_t address, uint32_t value) {
    size_t i = off(address);
    ram[i] = (uint8_t)value;
    ram[i + 1] = (uint8_t)(value >> 8);
    ram[i + 2] = (uint8_t)(value >> 16);
    ram[i + 3] = (uint8_t)(value >> 24);
}
void psx_mod_write_code_word(uint32_t address, uint32_t value) { psx_mod_write_word(address, value); }
uint32_t psx_mod_alloc_guest_memory(uint32_t size, uint32_t alignment) { (void)size; (void)alignment; return 0; }
uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t size, uint32_t alignment) { (void)size; (void)alignment; return 0; }
int psx_mod_game_started(void) { return 1; }
int32_t psx_mod_widescreen_x_margin(void) { return ws_margin; }
uint32_t psx_mod_display_width(void) { return display_width; }
uint32_t psx_mod_display_height(void) { return 240; }
int psx_mod_option_value(const char *package_id, const char *feature_id, const char *option_id, char *out, uint32_t out_size) { (void)package_id; (void)feature_id; (void)option_id; if (out && out_size) out[0] = 0; return 0; }
int psx_mod_current_resource_path(const char *resource_id, char *out, uint32_t out_size) { (void)resource_id; if (out && out_size) out[0] = 0; return 0; }
int psx_mod_set_fixed_display_aspect(uint32_t numerator, uint32_t denominator) { (void)numerator; (void)denominator; fixed_aspect_calls++; return 1; }
int psx_mod_set_adaptive_display_aspect(uint32_t max_numerator, uint32_t denominator) { (void)max_numerator; (void)denominator; adaptive_aspect_calls++; return 1; }
int psx_mod_set_native_vblank_rate(uint32_t frames_per_second) { (void)frames_per_second; return 1; }
int psx_mod_set_frame_interpolation(uint32_t frames_per_second) { (void)frames_per_second; return 1; }
int psx_mod_set_frame_interpolation_blend(uint32_t blend_mode) { (void)blend_mode; return 1; }
int psx_mod_set_auto_skip_fmv(int enabled) { (void)enabled; return 1; }
int psx_mod_set_bezel_artwork(const char *path) { (void)path; return 1; }
int psx_mod_set_load_acceleration(uint32_t wall_clock_multiplier, uint32_t read_speed_multiplier) { (void)wall_clock_multiplier; (void)read_speed_multiplier; return 1; }
int psx_mod_set_disc_speed(uint32_t divisor, uint32_t seek_divisor) { (void)divisor; (void)seek_divisor; return 1; }

int psx_mod_register_activation_plugin(const char *id, void (*callback)(void)) { (void)id; (void)callback; activation_regs++; return 1; }
int psx_mod_register_vblank_plugin(const char *id, void (*callback)(void)) { (void)id; (void)callback; return 1; }
int psx_mod_register_function_entry_plugin(const char *id, uint32_t address, void (*callback)(CPUState *, uint32_t)) { (void)id; (void)address; (void)callback; entry_regs++; return 1; }
void psx_mod_function_entry(CPUState *cpu, uint32_t address) { (void)cpu; (void)address; }

void gpu_ws_tag_screen_prim(uint32_t packet, int32_t anchor) {
    if (tag_count < (int)(sizeof(tags) / sizeof(tags[0]))) {
        tags[tag_count].packet = packet;
        tags[tag_count].anchor = anchor;
        tag_count++;
    }
}

void gpu_ws_tag_stretched_prim(uint32_t packet, int left_dst, int right_dst,
                               int left_src, int right_src) {
    if (stretch_count < (int)(sizeof(stretches) / sizeof(stretches[0]))) {
        stretches[stretch_count].packet = packet;
        stretches[stretch_count].left_dst = left_dst;
        stretches[stretch_count].right_dst = right_dst;
        stretches[stretch_count].left_src = left_src;
        stretches[stretch_count].right_src = right_src;
        stretch_count++;
    }
}

#include "../src/mods/toshinden_widescreen_plugin.c"

static void reset_scene(void) {
    memset(ram, 0, sizeof(ram));
    memset(tags, 0, sizeof(tags));
    memset(stretches, 0, sizeof(stretches));
    tag_count = 0;
    stretch_count = 0;
}

static uint32_t link24(uint32_t address) { return address & 0x00FFFFFFu; }

static void write_rect(uint32_t packet, int16_t x, int16_t y, int16_t w, int16_t h) {
    psx_mod_write_word(packet, 0); /* packet tag filled by caller */
    psx_mod_write_word(packet + 4u, 0x60000000u); /* variable-size untextured rect */
    psx_mod_write_half(packet + 8u, (uint16_t)x);
    psx_mod_write_half(packet + 10u, (uint16_t)y);
    psx_mod_write_half(packet + 12u, (uint16_t)w);
    psx_mod_write_half(packet + 14u, (uint16_t)h);
}

static void write_sprt(uint32_t packet, int16_t x, int16_t y, int16_t w, int16_t h) {
    psx_mod_write_word(packet, 0); /* packet tag filled by caller */
    psx_mod_write_word(packet + 4u, 0x64000000u); /* variable-size textured SPRT */
    psx_mod_write_half(packet + 8u, (uint16_t)x);
    psx_mod_write_half(packet + 10u, (uint16_t)y);
    psx_mod_write_word(packet + 12u, 0); /* UV/CLUT not relevant to anchoring */
    psx_mod_write_half(packet + 16u, (uint16_t)w);
    psx_mod_write_half(packet + 18u, (uint16_t)h);
}

static void write_quad39(uint32_t packet, int16_t x, int16_t y, int16_t w, int16_t h) {
    psx_mod_write_word(packet, 0);
    psx_mod_write_word(packet + 4u, 0x39000000u);
    psx_mod_write_half(packet + 8u, (uint16_t)x);
    psx_mod_write_half(packet + 10u, (uint16_t)y);
    psx_mod_write_word(packet + 12u, 0);
    psx_mod_write_half(packet + 16u, (uint16_t)(x + w));
    psx_mod_write_half(packet + 18u, (uint16_t)y);
    psx_mod_write_word(packet + 20u, 0);
    psx_mod_write_half(packet + 24u, (uint16_t)(x + w));
    psx_mod_write_half(packet + 26u, (uint16_t)(y + h));
    psx_mod_write_word(packet + 28u, 0);
    psx_mod_write_half(packet + 32u, (uint16_t)x);
    psx_mod_write_half(packet + 34u, (uint16_t)(y + h));
}

static void append_packet_words(uint32_t packet, uint32_t words, uint32_t next) {
    psx_mod_write_word(packet, (words << 24) | link24(next));
}

static void append_packet(uint32_t packet, uint32_t next) {
    append_packet_words(packet, 3u, next);
}

static void run_final_ot(uint32_t first_packet) {
    CPUState cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.gpr[4] = OT_HEAD0;
    psx_mod_write_word(OT_HEAD0, link24(first_packet));
    toshinden_widescreen_16_9_activate();
    toshinden_final_ot_entry(&cpu, FINAL_OT);
}

static int find_tag(uint32_t packet, int32_t *anchor) {
    int i;
    for (i = 0; i < tag_count; ++i) {
        if (tags[i].packet == packet) {
            if (anchor) *anchor = tags[i].anchor;
            return 1;
        }
    }
    return 0;
}

static int find_stretch(uint32_t packet) {
    int i;
    for (i = 0; i < stretch_count; ++i)
        if (stretches[i].packet == packet) return 1;
    return 0;
}

static void expect_anchor(uint32_t packet, int32_t expected, const char *msg) {
    int32_t anchor = -9999;
    CHECK(find_tag(packet, &anchor), msg);
    CHECK(anchor == expected, msg);
}

static void test_score_digits_center_anchor(void) {
    uint32_t packets[2u * SCORE_DIGITS * 2u];
    uint32_t n = 0;
    uint32_t bank, player, digit;
    uint32_t next;

    reset_scene();
    for (bank = 0; bank < 2u; ++bank) {
        uint32_t bank_base = BATTLE_UI_BASE + bank * BATTLE_UI_STRIDE;
        for (player = 0; player < 2u; ++player) {
            uint32_t pool = bank_base + SCORE_POOL_LO + player * SCORE_DIGITS * SCORE_GLYPH_STRIDE;
            for (digit = 0; digit < SCORE_DIGITS; ++digit) {
                uint32_t packet = pool + digit * SCORE_GLYPH_STRIDE;
                int16_t x = (player == 0u) ? (int16_t)(160 + (int)digit * 16) :
                                             (int16_t)(352 + (int)digit * 16);
                write_sprt(packet, x, 16, 16, 16);
                packets[n++] = packet;
            }
        }
    }
    for (uint32_t i = 0; i < n; ++i) {
        next = (i + 1u < n) ? packets[i + 1u] : 0x80FFFFFFu;
        append_packet_words(packets[i], 4u, next);
    }
    run_final_ot(packets[0]);

    CHECK(tag_count == (int)n, "all P1/P2 score digits in both banks are tagged");
    for (uint32_t i = 0; i < n; ++i)
        expect_anchor(packets[i], CENTER_X, "score digit uses center anchor despite left/right x deadzone");
}

static void test_neighbor_roles_keep_existing_behavior(void) {
    const uint32_t bank = BATTLE_UI_BASE;
    const uint32_t health_l = bank + 0x78u;
    const uint32_t health_r = bank + 0x12Cu;
    const uint32_t p1_name = bank + 0x340u;
    const uint32_t p2_name = bank + 0x380u;
    const uint32_t timer = bank + 0x420u;
    const uint32_t demo = bank + 0x1428u;
    uint32_t packets[] = { health_l, health_r, p1_name, p2_name, timer, demo };

    reset_scene();
    write_quad39(health_l, 40, 12, 80, 8);
    write_quad39(health_r, 200, 12, 80, 8);
    write_rect(p1_name, 40, 24, 32, 8);
    write_rect(p2_name, 560, 24, 32, 8);
    write_rect(timer, 310, 8, 20, 12);
    write_rect(demo, 112, 32, 96, 16);
    for (uint32_t i = 0; i < sizeof(packets)/sizeof(packets[0]); ++i) {
        uint32_t words = (packets[i] == health_l || packets[i] == health_r) ? 9u : 3u;
        append_packet_words(packets[i], words,
                            (i + 1u < sizeof(packets)/sizeof(packets[0])) ? packets[i + 1u] : 0x80FFFFFFu);
    }
    run_final_ot(packets[0]);

    CHECK(find_stretch(health_l), "left health gauge remains stretched");
    CHECK(find_stretch(health_r), "right health gauge remains stretched");
    CHECK(!find_tag(health_l, NULL), "left health gauge is not screen-anchored");
    CHECK(!find_tag(health_r, NULL), "right health gauge is not screen-anchored");
    expect_anchor(p1_name, 0, "left-side name remains left anchored");
    expect_anchor(p2_name, (int32_t)DISPLAY_W, "right-side name remains right anchored");
    expect_anchor(timer, CENTER_X, "timer remains center anchored");
    expect_anchor(demo, CENTER_X, "demo/pause overlay pool remains center anchored");
}

static void test_disabled_and_4x3_passthrough(void) {
    const uint32_t packet = BATTLE_UI_BASE + SCORE_POOL_LO;
    CPUState cpu;

    reset_scene();
    write_rect(packet, 40, 16, 8, 8);
    append_packet(packet, 0x80FFFFFFu);
    memset(&cpu, 0, sizeof(cpu));
    cpu.gpr[4] = OT_HEAD0;
    psx_mod_write_word(OT_HEAD0, link24(packet));
    s_widescreen_enabled = 0;
    toshinden_final_ot_entry(&cpu, FINAL_OT);
    CHECK(tag_count == 0 && stretch_count == 0, "mod disabled passes through without tags");

    reset_scene();
    write_rect(packet, 40, 16, 8, 8);
    append_packet(packet, 0x80FFFFFFu);
    cpu.gpr[4] = OT_HEAD0;
    psx_mod_write_word(OT_HEAD0, link24(packet));
    s_widescreen_enabled = 1;
    ws_margin = 0;
    toshinden_final_ot_entry(&cpu, FINAL_OT);
    CHECK(tag_count == 0 && stretch_count == 0, "4:3 zero-margin passes through without tags");
    ws_margin = 160;
}

int main(void) {
    toshinden_register_widescreen_plugins();
    CHECK(activation_regs == 2, "constructor registers both activation plugins");
    CHECK(entry_regs == 1, "constructor registers final OT hook");
    test_score_digits_center_anchor();
    test_neighbor_roles_keep_existing_behavior();
    test_disabled_and_4x3_passthrough();
    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("ALL PASS\n");
    return 0;
}
