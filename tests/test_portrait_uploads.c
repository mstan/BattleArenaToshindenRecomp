#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_state.h"
#include "mod_plugins.h"

#define RAM_BASE 0x80000000u
#define RAM_SIZE 0x00200000u
#define MOD_DMA_BASE 0x80F00000u
#define MOD_DMA_SIZE 0x00100000u

#define FINAL_OT 0x80197C70u
#define SELECT_SCENE 0x801CA584u
#define SELECT_STATE_OFFSET 0x154u
#define PLAYER_1 0x801BC114u
#define PLAYER_2 0x801BC1E8u
#define PLAYER_MODE_OFFSET 0x02u
#define PLAYER_CHAR_OFFSET 0x06u
#define OT_HEAD0 0x801F25C0u
#define NATIVE_ENV_PACKET 0x801F2640u
#define NATIVE_HILITE_0_BASE 0x801F28ECu
#define NATIVE_HILITE_STRIDE 0x28u
#define NATIVE_VS_LOGO 0x801F2B1Cu
#define NATIVE_CURSOR_0 0x801F2B44u
#define NATIVE_CURSOR_1 0x801F2B54u
#define NATIVE_LABEL_0_A 0x801F2A2Cu
#define NATIVE_LABEL_0_B 0x801F2A54u
#define NATIVE_LABEL_1_A 0x801F2A7Cu
#define NATIVE_LABEL_1_B 0x801F2AA4u
#define NATIVE_COM_LABEL_A 0x801F2ACCu
#define NATIVE_COM_LABEL_B 0x801F2AF4u
#define OT_TAIL 0x801F2E00u
#define LINK_END 0x00FFFFFFu

#define GAIA_X 640u
#define GAIA_Y 128u
#define SHO_X 768u
#define SHO_Y 128u
#define PORTRAIT_W 128u
#define PORTRAIT_H 128u
#define VRAM_W 1024u
#define VRAM_H 512u

static uint8_t ram[RAM_SIZE];
static uint8_t mod_dma[MOD_DMA_SIZE];
static uint16_t vram[VRAM_W * VRAM_H];
static uint16_t original_band[PORTRAIT_W * PORTRAIT_H * 2u];
static uint32_t mod_dma_used;
static int failures;
static int activation_regs;
static int entry_regs;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
    else         { printf("ok:   %s\n", msg); } \
} while (0)

static uint8_t *ptr_for(uint32_t address, uint32_t width) {
    if (address >= MOD_DMA_BASE && address <= MOD_DMA_BASE + MOD_DMA_SIZE &&
        width <= MOD_DMA_BASE + MOD_DMA_SIZE - address) {
        return mod_dma + (address - MOD_DMA_BASE);
    }
    if (address >= RAM_BASE && address <= RAM_BASE + RAM_SIZE &&
        width <= RAM_BASE + RAM_SIZE - address) {
        return ram + (address - RAM_BASE);
    }
    printf("FAIL: address out of test memory 0x%08x width %u\n", address, width);
    exit(2);
}

uint8_t psx_mod_read_byte(uint32_t address) {
    return *ptr_for(address, 1u);
}

void psx_mod_write_byte(uint32_t address, uint8_t value) {
    *ptr_for(address, 1u) = value;
}

uint16_t psx_mod_read_half(uint32_t address) {
    uint8_t *p = ptr_for(address, 2u);
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

void psx_mod_write_half(uint32_t address, uint16_t value) {
    uint8_t *p = ptr_for(address, 2u);
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

uint32_t psx_mod_read_word(uint32_t address) {
    uint8_t *p = ptr_for(address, 4u);
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void psx_mod_write_word(uint32_t address, uint32_t value) {
    uint8_t *p = ptr_for(address, 4u);
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

void psx_mod_write_code_word(uint32_t address, uint32_t value) {
    psx_mod_write_word(address, value);
}

uint32_t psx_mod_alloc_guest_memory(uint32_t size, uint32_t alignment) {
    (void)size;
    (void)alignment;
    return 0;
}

uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t size, uint32_t alignment) {
    uint32_t start;
    if (alignment == 0u)
        alignment = 1u;
    start = (mod_dma_used + alignment - 1u) & ~(alignment - 1u);
    if (start > MOD_DMA_SIZE || size > MOD_DMA_SIZE - start)
        return 0;
    memset(mod_dma + start, 0, size);
    mod_dma_used = start + size;
    return MOD_DMA_BASE + start;
}

int psx_mod_register_activation_plugin(const char *id,
                                       PSXModActivationCallback callback) {
    (void)id;
    (void)callback;
    activation_regs++;
    return 1;
}

int psx_mod_register_vblank_plugin(const char *id,
                                   PSXModVBlankCallback callback) {
    (void)id;
    (void)callback;
    return 1;
}

int psx_mod_register_function_entry_plugin(const char *id, uint32_t address,
                                           PSXModFunctionEntryCallback callback) {
    (void)id;
    (void)address;
    (void)callback;
    entry_regs++;
    return 1;
}

void psx_mod_function_entry(struct CPUState *cpu, uint32_t address) {
    (void)cpu;
    (void)address;
}

int psx_mod_game_started(void) { return 1; }
int32_t psx_mod_widescreen_x_margin(void) { return 0; }
uint32_t psx_mod_display_width(void) { return 640u; }
uint32_t psx_mod_display_height(void) { return 240u; }
int psx_mod_option_value(const char *package_id, const char *feature_id,
                         const char *option_id, char *out, uint32_t out_size) {
    (void)package_id;
    (void)feature_id;
    (void)option_id;
    if (out != NULL && out_size != 0u)
        out[0] = '\0';
    return 0;
}
int psx_mod_current_resource_path(const char *resource_id, char *out,
                                  uint32_t out_size) {
    (void)resource_id;
    if (out != NULL && out_size != 0u)
        out[0] = '\0';
    return 0;
}
int psx_mod_set_fixed_display_aspect(uint32_t numerator, uint32_t denominator) {
    (void)numerator;
    (void)denominator;
    return 1;
}
int psx_mod_set_adaptive_display_aspect(uint32_t max_numerator,
                                        uint32_t denominator) {
    (void)max_numerator;
    (void)denominator;
    return 1;
}
int psx_mod_set_native_vblank_rate(uint32_t frames_per_second) {
    (void)frames_per_second;
    return 1;
}
int psx_mod_set_frame_interpolation(uint32_t frames_per_second) {
    (void)frames_per_second;
    return 1;
}
int psx_mod_set_frame_interpolation_blend(uint32_t blend_mode) {
    (void)blend_mode;
    return 1;
}
int psx_mod_set_auto_skip_fmv(int enabled) {
    (void)enabled;
    return 1;
}
int psx_mod_set_bezel_artwork(const char *path) {
    (void)path;
    return 1;
}
int psx_mod_set_load_acceleration(uint32_t wall_clock_multiplier,
                                  uint32_t read_speed_multiplier) {
    (void)wall_clock_multiplier;
    (void)read_speed_multiplier;
    return 1;
}
int psx_mod_set_disc_speed(uint32_t divisor, uint32_t seek_divisor) {
    (void)divisor;
    (void)seek_divisor;
    return 1;
}

void gpu_ws_tag_screen_prim(uint32_t packet, int32_t anchor) {
    (void)packet;
    (void)anchor;
}

void gpu_ws_tag_stretched_prim(uint32_t packet, int left_dst, int right_dst,
                               int left_src, int right_src) {
    (void)packet;
    (void)left_dst;
    (void)right_dst;
    (void)left_src;
    (void)right_src;
}

const uint16_t *gpu_get_vram(void) {
    return vram;
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "../src/mods/toshinden_boss_portraits.c"
#include "../src/mods/toshinden_boss_roster_ui.c"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

typedef struct UploadRecord {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} UploadRecord;

typedef struct Gp0Probe {
    int in_upload;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    uint32_t col;
    uint32_t row;
    uint32_t remaining_words;
    uint32_t e2;
    uint32_t e6;
    UploadRecord uploads[8];
    uint32_t upload_count;
    uint32_t draw_count;
    uint32_t gaia_draw_saw_uploaded;
    uint32_t sho_draw_saw_uploaded;
    uint32_t max_node_words;
    uint32_t invalid_stream;
} Gp0Probe;

static uint32_t link24(uint32_t address) {
    return address & LINK_END;
}

static uint32_t xy(uint32_t x, uint32_t y) {
    return (y << 16) | x;
}

static uint32_t wh(uint32_t w, uint32_t h) {
    return (h << 16) | w;
}

static void write_packet(uint32_t packet, const uint32_t *words,
                         uint32_t word_count, uint32_t next) {
    uint32_t i;
    psx_mod_write_word(packet, (word_count << 24) | link24(next));
    for (i = 0; i < word_count; i++)
        psx_mod_write_word(packet + 4u + i * 4u, words[i]);
}

static void write_native_thumbnail(uint32_t packet, uint32_t next) {
    uint32_t words[9] = {
        0x2CFFFFFFu,
        xy(32u, 190u),
        0u,
        xy(64u, 190u),
        0u,
        xy(32u, 224u),
        0u,
        xy(64u, 224u),
        0u,
    };
    write_packet(packet, words, 9u, next);
}

static void write_native_label_like_packet(uint32_t packet, uint32_t next) {
    uint32_t words[9] = {
        0x2CFFFFFFu,
        xy(100u, 178u),
        0u,
        xy(117u, 178u),
        0u,
        xy(100u, 187u),
        0u,
        xy(117u, 187u),
        0u,
    };
    write_packet(packet, words, 9u, next);
}

static void write_clean_select_ot(uint32_t e2, uint32_t e6) {
    uint32_t env_words[2] = { e2, e6 };
    uint32_t i;

    write_packet(OT_HEAD0, NULL, 0u, NATIVE_ENV_PACKET);
    write_packet(NATIVE_ENV_PACKET, env_words, 2u, NATIVE_HILITE_0_BASE);
    for (i = 0; i < 8u; i++) {
        uint32_t packet = NATIVE_HILITE_0_BASE + i * NATIVE_HILITE_STRIDE;
        uint32_t next = (i == 7u) ? NATIVE_VS_LOGO :
            NATIVE_HILITE_0_BASE + (i + 1u) * NATIVE_HILITE_STRIDE;
        write_native_thumbnail(packet, next);
    }
    write_native_thumbnail(NATIVE_VS_LOGO, NATIVE_CURSOR_0);
    write_native_thumbnail(NATIVE_CURSOR_0, NATIVE_CURSOR_1);
    write_native_thumbnail(NATIVE_CURSOR_1, NATIVE_LABEL_0_A);
    write_native_thumbnail(NATIVE_LABEL_0_A, NATIVE_LABEL_0_B);
    write_native_thumbnail(NATIVE_LABEL_0_B, NATIVE_LABEL_1_A);
    write_native_thumbnail(NATIVE_LABEL_1_A, NATIVE_LABEL_1_B);
    write_native_thumbnail(NATIVE_LABEL_1_B, NATIVE_COM_LABEL_A);
    write_native_label_like_packet(NATIVE_COM_LABEL_A, NATIVE_COM_LABEL_B);
    write_native_label_like_packet(NATIVE_COM_LABEL_B, OT_TAIL);
    write_packet(OT_TAIL, NULL, 0u, LINK_END);
}

static void seed_vram(void) {
    uint32_t i;
    for (i = 0; i < VRAM_W * VRAM_H; i++)
        vram[i] = (uint16_t)(0x8000u | (i & 0x7FFFu));

    for (i = 0; i < PORTRAIT_W * PORTRAIT_H; i++) {
        original_band[i] = vram[(GAIA_Y + i / PORTRAIT_W) * VRAM_W +
                                GAIA_X + i % PORTRAIT_W];
        original_band[PORTRAIT_W * PORTRAIT_H + i] =
            vram[(SHO_Y + i / PORTRAIT_W) * VRAM_W + SHO_X + i % PORTRAIT_W];
    }
}

static void reset_scene(void) {
    memset(ram, 0, sizeof(ram));
    memset(mod_dma, 0, sizeof(mod_dma));
    mod_dma_used = 0;
    s_boss_ui_enabled = 0;
    s_dma_buffers[0] = 0;
    s_dma_buffers[1] = 0;
    s_restore_valid = 0;
    seed_vram();
}

static void write_select_state_and_players(void) {
    psx_mod_write_half(SELECT_SCENE + SELECT_STATE_OFFSET, 5u);
    psx_mod_write_half(PLAYER_1 + PLAYER_CHAR_OFFSET, TOSHINDEN_CHAR_GAIA);
    psx_mod_write_half(PLAYER_2 + PLAYER_CHAR_OFFSET, TOSHINDEN_CHAR_SHO);
    psx_mod_write_half(PLAYER_2 + PLAYER_MODE_OFFSET, 0u);
}

static void gp0_consume_upload_word(Gp0Probe *probe, uint32_t word) {
    uint32_t i;
    for (i = 0; i < 2u; i++) {
        uint16_t pixel = (uint16_t)(word >> (i * 16u));
        uint32_t wx = (probe->x + probe->col) & 1023u;
        uint32_t wy = (probe->y + probe->row) & 511u;
        int check_mask = (probe->e6 & 2u) != 0u;
        int set_mask = (probe->e6 & 1u) != 0u;

        if (!check_mask || (vram[wy * VRAM_W + wx] & 0x8000u) == 0u)
            vram[wy * VRAM_W + wx] = (uint16_t)(pixel | (set_mask ? 0x8000u : 0u));

        probe->col++;
        if (probe->col == probe->w) {
            probe->col = 0;
            probe->row++;
            if (probe->row == probe->h) {
                probe->in_upload = 0;
                probe->remaining_words = 0;
                return;
            }
        }
    }
    probe->remaining_words--;
    if (probe->remaining_words == 0u)
        probe->in_upload = 0;
}

static int fixed_word_count(uint32_t word) {
    uint32_t op = word >> 24;
    if (op <= 0x1Fu) return op == 0x02u ? 3 : 1;
    if (op >= 0x20u && op <= 0x23u) return 4;
    if (op >= 0x24u && op <= 0x27u) return 7;
    if (op >= 0x28u && op <= 0x2Bu) return 5;
    if (op >= 0x2Cu && op <= 0x2Fu) return 9;
    if (op >= 0x30u && op <= 0x33u) return 6;
    if (op >= 0x34u && op <= 0x37u) return 9;
    if (op >= 0x38u && op <= 0x3Bu) return 8;
    if (op >= 0x3Cu && op <= 0x3Fu) return 12;
    if (op >= 0x40u && op <= 0x47u) return 3;
    if (op >= 0x50u && op <= 0x57u) return 4;
    if (op >= 0x60u && op <= 0x63u) return 3;
    if (op >= 0x64u && op <= 0x67u) return 4;
    if (op >= 0x68u && op <= 0x6Bu) return 2;
    if (op >= 0x6Cu && op <= 0x6Fu) return 3;
    if (op >= 0x70u && op <= 0x73u) return 2;
    if (op >= 0x74u && op <= 0x77u) return 3;
    if (op >= 0x78u && op <= 0x7Bu) return 2;
    if (op >= 0x7Cu && op <= 0x7Fu) return 3;
    if (op >= 0x80u && op <= 0xDFu) return 4;
    if (op >= 0xE0u && op <= 0xEFu) return 1;
    return 0;
}

static void observe_draw(Gp0Probe *probe, const uint32_t *words) {
    uint32_t op = words[0] >> 24;
    uint16_t tpage;
    uint32_t u;
    uint32_t v;
    uint32_t sx;
    uint32_t sy;

    if (op < 0x2Cu || op > 0x2Fu)
        return;

    probe->draw_count++;
    tpage = (uint16_t)(words[4] >> 16);
    u = words[2] & 0xFFu;
    v = (words[2] >> 8) & 0xFFu;
    sx = ((uint32_t)(tpage & 0x0Fu) * 64u + u) & 1023u;
    sy = (((tpage & 0x10u) != 0u ? 256u : 0u) + v) & 511u;

    if (sx == GAIA_X && sy == GAIA_Y &&
        vram[sy * VRAM_W + sx] == toshinden_gaia_portrait[0]) {
        probe->gaia_draw_saw_uploaded = 1u;
    }
    if (sx == SHO_X && sy == SHO_Y &&
        vram[sy * VRAM_W + sx] == toshinden_sho_portrait[0]) {
        probe->sho_draw_saw_uploaded = 1u;
    }
}

static void execute_packet_words(Gp0Probe *probe, uint32_t packet,
                                 uint32_t word_count) {
    uint32_t index = 0;
    if (word_count > probe->max_node_words)
        probe->max_node_words = word_count;

    while (index < word_count) {
        uint32_t word = psx_mod_read_word(packet + 4u + index * 4u);
        uint32_t op = word >> 24;
        int count;

        if (probe->in_upload) {
            gp0_consume_upload_word(probe, word);
            index++;
            continue;
        }

        if (op == 0xE2u) {
            probe->e2 = word;
            index++;
            continue;
        }
        if (op == 0xE6u) {
            probe->e6 = word;
            index++;
            continue;
        }
        if (op >= 0xA0u && op <= 0xBFu) {
            uint32_t xy_word;
            uint32_t wh_word;
            if (index + 2u >= word_count) {
                probe->invalid_stream++;
                return;
            }
            xy_word = psx_mod_read_word(packet + 4u + (index + 1u) * 4u);
            wh_word = psx_mod_read_word(packet + 4u + (index + 2u) * 4u);
            probe->x = xy_word & 0x3FFu;
            probe->y = (xy_word >> 16) & 0x1FFu;
            probe->w = wh_word & 0x3FFu;
            probe->h = (wh_word >> 16) & 0x1FFu;
            if (probe->w == 0u) probe->w = 1024u;
            if (probe->h == 0u) probe->h = 512u;
            probe->col = 0;
            probe->row = 0;
            probe->remaining_words = (probe->w * probe->h + 1u) / 2u;
            probe->in_upload = 1;
            if (probe->upload_count < 8u) {
                probe->uploads[probe->upload_count].x = probe->x;
                probe->uploads[probe->upload_count].y = probe->y;
                probe->uploads[probe->upload_count].w = probe->w;
                probe->uploads[probe->upload_count].h = probe->h;
            }
            probe->upload_count++;
            index += 3u;
            continue;
        }

        count = fixed_word_count(word);
        if (count <= 0 || index + (uint32_t)count > word_count) {
            probe->invalid_stream++;
            return;
        }
        if (op >= 0x20u && op <= 0x7Fu) {
            uint32_t words[12];
            uint32_t i;
            for (i = 0; i < (uint32_t)count; i++)
                words[i] = psx_mod_read_word(packet + 4u + (index + i) * 4u);
            observe_draw(probe, words);
        }
        index += (uint32_t)count;
    }
}

static void execute_ot(uint32_t head, Gp0Probe *probe) {
    uint32_t node = head;
    uint32_t safety;
    for (safety = 0; safety < 8192u; safety++) {
        uint32_t tag = psx_mod_read_word(node);
        uint32_t word_count = tag >> 24;
        uint32_t next = tag & LINK_END;
        execute_packet_words(probe, node, word_count);
        if (next == LINK_END)
            return;
        node = 0x80000000u | next;
    }
    CHECK(0, "OT traversal terminates");
}

static void expect_original_vram_restored(void) {
    uint32_t i;
    for (i = 0; i < PORTRAIT_W * PORTRAIT_H; i++) {
        uint16_t gaia = vram[(GAIA_Y + i / PORTRAIT_W) * VRAM_W +
                             GAIA_X + i % PORTRAIT_W];
        uint16_t sho = vram[(SHO_Y + i / PORTRAIT_W) * VRAM_W +
                            SHO_X + i % PORTRAIT_W];
        if (gaia != original_band[i] ||
            sho != original_band[PORTRAIT_W * PORTRAIT_H + i]) {
            CHECK(0, "portrait VRAM bands restored exactly");
            return;
        }
    }
    CHECK(1, "portrait VRAM bands restored exactly");
}

static void test_transient_upload_draw_restore_stream(void) {
    CPUState cpu;
    Gp0Probe probe;
    uint32_t appended;

    reset_scene();
    write_select_state_and_players();
    write_clean_select_ot(0xE2001234u, 0xE6000003u);
    toshinden_boss_roster_ui_activate();

    memset(&cpu, 0, sizeof(cpu));
    cpu.gpr[4] = OT_HEAD0;
    toshinden_boss_roster_ui_final_ot(&cpu, FINAL_OT);

    appended = psx_mod_read_word(OT_TAIL) & LINK_END;
    CHECK(appended != LINK_END, "final OT appends mod chain on safe native OT");

    memset(&probe, 0, sizeof(probe));
    execute_ot(OT_HEAD0, &probe);

    CHECK(probe.upload_count == 4u, "stream emits Gaia/Sho uploads and restores");
    CHECK(probe.uploads[0].x == GAIA_X && probe.uploads[0].y == GAIA_Y,
          "first upload targets Gaia texture slot");
    CHECK(probe.uploads[1].x == SHO_X && probe.uploads[1].y == SHO_Y,
          "second upload targets Sho texture slot");
    CHECK(probe.uploads[2].x == GAIA_X && probe.uploads[2].y == GAIA_Y,
          "third upload restores Gaia native slot");
    CHECK(probe.uploads[3].x == SHO_X && probe.uploads[3].y == SHO_Y,
          "fourth upload restores Sho native slot");
    CHECK(probe.max_node_words <= 255u, "all emitted OT nodes stay within 255 payload words");
    CHECK(probe.invalid_stream == 0u, "emitted stream parses as valid GP0 packets/data");
    CHECK(probe.gaia_draw_saw_uploaded != 0u, "Gaia draw samples uploaded portrait before restore");
    CHECK(probe.sho_draw_saw_uploaded != 0u, "Sho draw samples uploaded portrait before restore");
    CHECK(probe.e6 == 0xE6000003u, "stream restores native E6 mask state");
    CHECK(probe.e2 == 0xE2001234u, "stream restores native E2 texture-window state");
    CHECK(probe.in_upload == 0, "stream finishes all A0 payloads");
    expect_original_vram_restored();
}

static void test_native_ot_rejects_vram_changing_packets(void) {
    uint32_t e2;
    uint32_t e6;
    uint32_t bad_a0[3] = { 0xA0000000u, xy(640u, 128u), wh(1u, 1u) };
    uint32_t bad_poly[3] = { 0x48000000u, xy(1u, 1u), 0x55555555u };

    reset_scene();
    write_clean_select_ot(0xE2000000u, 0xE6000000u);
    write_packet(NATIVE_ENV_PACKET, bad_a0, 3u, NATIVE_HILITE_0_BASE);
    CHECK(!toshinden_ot_is_safe_for_transient_vram(OT_HEAD0, &e2, &e6),
          "native OT safety rejects A0 upload packets");

    reset_scene();
    write_clean_select_ot(0xE2000000u, 0xE6000000u);
    write_packet(NATIVE_ENV_PACKET, bad_poly, 3u, NATIVE_HILITE_0_BASE);
    CHECK(!toshinden_ot_is_safe_for_transient_vram(OT_HEAD0, &e2, &e6),
          "native OT safety rejects variable-length polylines");

    reset_scene();
    write_clean_select_ot(0xE20000A5u, 0xE6000002u);
    CHECK(toshinden_ot_is_safe_for_transient_vram(OT_HEAD0, &e2, &e6),
          "native OT safety accepts fixed draw/env-only packets");
    CHECK(e2 == 0xE20000A5u && e6 == 0xE6000002u,
          "native OT safety captures E2/E6 restore state");
}

static void test_builder_overflow_fails_closed(void) {
    ToshindenUiBuilder builder;
    uint32_t words[255];

    reset_scene();
    memset(words, 0x5A, sizeof(words));
    toshinden_builder_init(&builder, MOD_DMA_BASE);
    builder.end = builder.base + 32u;

    CHECK(!toshinden_emit_boss_portrait_uploads(&builder),
          "portrait upload builder reports overflow");
    CHECK(builder.failed != 0, "builder overflow latches failure");
    CHECK(!toshinden_emit_words(&builder, words, 1u),
          "builder rejects further packets after overflow");
}

int main(void) {
    toshinden_register_boss_roster_ui_plugins();
    CHECK(activation_regs == 1, "constructor registers boss UI activation plugin");
    CHECK(entry_regs == 1, "constructor registers boss UI final-OT hook");
    test_transient_upload_draw_restore_stream();
    test_native_ot_rejects_vram_changing_packets();
    test_builder_overflow_fails_closed();
    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    puts("ALL PASS");
    return 0;
}
