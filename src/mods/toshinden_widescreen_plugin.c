#include "cpu_state.h"
#include "gpu.h"
#include "mod_plugins.h"

#include <stdint.h>

#define TOSHINDEN_FINAL_OT_SUBMIT          0x80197C70u

#define TOSHINDEN_MAIN_RAM_BASE            0x80000000u
#define TOSHINDEN_MAIN_RAM_END             0x80200000u
#define TOSHINDEN_LINK_TERMINATOR          0x00FFFFFFu
#define TOSHINDEN_OT_WORDS                 0x800u
#define TOSHINDEN_OT_HEAD_DELTA            ((TOSHINDEN_OT_WORDS - 1u) * 4u)
#define TOSHINDEN_OT_WALK_LIMIT            8192u

#define TOSHINDEN_BATTLE_UI_BASE           0x801BC2BCu
#define TOSHINDEN_BATTLE_UI_END            0x801C5914u
#define TOSHINDEN_BATTLE_UI_STRIDE         0x4B28u

#define TOSHINDEN_PAUSE_PANEL_BUF0_BASE    0x801F2B44u
#define TOSHINDEN_PAUSE_PANEL_BUF1_BASE    0x801F5454u
#define TOSHINDEN_PAUSE_PANEL_SPAN         0x00000060u

#define TOSHINDEN_MENU_TEXT_Y_MIN          48
#define TOSHINDEN_MENU_TEXT_Y_MAX          224
#define TOSHINDEN_TOP_HUD_Y_MIN            -8
#define TOSHINDEN_TOP_HUD_Y_MAX            96
#define TOSHINDEN_CENTER_DEADZONE          64

static int s_widescreen_enabled;

static int toshinden_is_main_ram(uint32_t address) {
    return address >= TOSHINDEN_MAIN_RAM_BASE &&
           address < TOSHINDEN_MAIN_RAM_END;
}

static uint32_t toshinden_link_to_ram(uint32_t link) {
    if ((link & 0x00FFFFFFu) == TOSHINDEN_LINK_TERMINATOR)
        return 0;
    return TOSHINDEN_MAIN_RAM_BASE | (link & 0x00FFFFFFu);
}

static int toshinden_in_span(uint32_t address, uint32_t base,
                             uint32_t span) {
    return address >= base && address < base + span;
}

static int toshinden_is_battle_ui_packet(uint32_t packet) {
    return packet >= TOSHINDEN_BATTLE_UI_BASE &&
           packet < TOSHINDEN_BATTLE_UI_END;
}

static int toshinden_is_pause_panel_packet(uint32_t packet) {
    return toshinden_in_span(packet, TOSHINDEN_PAUSE_PANEL_BUF0_BASE,
                             TOSHINDEN_PAUSE_PANEL_SPAN) ||
           toshinden_in_span(packet, TOSHINDEN_PAUSE_PANEL_BUF1_BASE,
                             TOSHINDEN_PAUSE_PANEL_SPAN);
}

static int toshinden_is_options_packet(uint32_t packet) {
    /* Controls/options buffers initialized by 801827A0 and emitted by
     * 8017F7B0, including options opened over a paused 3D battle. */
    return toshinden_in_span(packet, 0x801C5914u, 2u * 0x1040u) ||
           toshinden_in_span(packet, 0x801D3474u, 2u * 0x00A0u) ||
           toshinden_in_span(packet, 0x801D63F4u, 2u * 0x2A30u);
}

static int toshinden_poly_command_words(uint8_t op) {
    uint32_t verts;
    uint32_t gouraud;
    uint32_t textured;
    uint32_t words;

    if (op < 0x20u || op > 0x3Fu)
        return 0;

    verts = (op & 0x08u) ? 4u : 3u;
    gouraud = (op & 0x10u) ? 1u : 0u;
    textured = (op & 0x04u) ? 1u : 0u;

    words = 1u + verts;
    if (textured)
        words += verts;
    if (gouraud)
        words += verts - 1u;
    return (int)words;
}

static int toshinden_rect_command_words(uint8_t op) {
    uint32_t size_selector;

    if (op < 0x60u || op > 0x7Fu)
        return 0;

    size_selector = (op >> 3) & 3u;
    if (size_selector == 0u)
        return (op & 0x04u) ? 4 : 3;
    return (op & 0x04u) ? 3 : 2;
}

static int toshinden_gp0_command_words(uint8_t op) {
    if (op >= 0x20u && op <= 0x3Fu)
        return toshinden_poly_command_words(op);
    if (op >= 0x60u && op <= 0x7Fu)
        return toshinden_rect_command_words(op);

    switch (op) {
        case 0x00: return 1;
        case 0x01: return 1;
        case 0x02: return 3;
        case 0x1F: return 1;
        case 0xE1: return 1;
        case 0xE2: return 1;
        case 0xE3: return 1;
        case 0xE4: return 1;
        case 0xE5: return 1;
        case 0xE6: return 1;
        default: return 0;
    }
}

static int toshinden_rect_extent(uint32_t packet, uint8_t op,
                                 int32_t *min_x, int32_t *max_x,
                                 int32_t *min_y, int32_t *max_y) {
    uint32_t size_selector;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    if (op < 0x60u || op > 0x7Fu)
        return 0;

    size_selector = (op >> 3) & 3u;
    x = (int16_t)psx_mod_read_half(packet + 8u);
    y = (int16_t)psx_mod_read_half(packet + 10u);

    if (size_selector == 0u) {
        uint32_t size_offset = (op & 0x04u) ? 16u : 12u;
        width = (int32_t)(int16_t)psx_mod_read_half(packet + size_offset);
        height = (int32_t)(int16_t)psx_mod_read_half(packet + size_offset + 2u);
    } else if (size_selector == 1u) {
        width = 1;
        height = 1;
    } else if (size_selector == 2u) {
        width = 8;
        height = 8;
    } else {
        width = 16;
        height = 16;
    }

    if (width <= 0 || height <= 0)
        return 0;

    *min_x = x;
    *max_x = x + width;
    *min_y = y;
    *max_y = y + height;
    return 1;
}

static int toshinden_poly_extent(uint32_t packet, uint8_t op,
                                 int32_t *min_x, int32_t *max_x,
                                 int32_t *min_y, int32_t *max_y) {
    uint32_t vertex_stride;
    int words;
    int16_t xs[4];
    int16_t ys[4];
    uint32_t i;

    if ((op & 0x08u) == 0u)
        return 0;

    words = toshinden_poly_command_words(op);
    if (words <= 0 || words > 13)
        return 0;

    vertex_stride = 1u + ((op & 0x10u) ? 1u : 0u) +
                    ((op & 0x04u) ? 1u : 0u);
    for (i = 0; i < 4u; ++i) {
        uint32_t word_offset = 4u + (1u + i * vertex_stride) * 4u;
        xs[i] = (int16_t)psx_mod_read_half(packet + word_offset);
        ys[i] = (int16_t)psx_mod_read_half(packet + word_offset + 2u);
    }

    *min_x = *max_x = xs[0];
    *min_y = *max_y = ys[0];
    for (i = 1; i < 4u; ++i) {
        if (xs[i] < *min_x) *min_x = xs[i];
        if (xs[i] > *max_x) *max_x = xs[i];
        if (ys[i] < *min_y) *min_y = ys[i];
        if (ys[i] > *max_y) *max_y = ys[i];
    }

    if (*max_x <= *min_x || *max_y <= *min_y)
        return 0;
    return 1;
}

static int toshinden_packet_extent(uint32_t packet, uint8_t op,
                                   int32_t *min_x, int32_t *max_x,
                                   int32_t *min_y, int32_t *max_y) {
    if (op >= 0x20u && op <= 0x3Fu)
        return toshinden_poly_extent(packet, op, min_x, max_x, min_y, max_y);
    if (op >= 0x60u && op <= 0x7Fu)
        return toshinden_rect_extent(packet, op, min_x, max_x, min_y, max_y);
    return 0;
}

static int32_t toshinden_anchor_for_packet(uint32_t packet,
                                           int32_t min_x, int32_t max_x,
                                           int32_t min_y,
                                           uint32_t display_width) {
    int32_t center = (int32_t)display_width / 2;
    int32_t packet_center = (min_x + max_x) / 2;
    uint32_t role = (packet - TOSHINDEN_BATTLE_UI_BASE) %
                    TOSHINDEN_BATTLE_UI_STRIDE;

    if (min_y >= TOSHINDEN_MENU_TEXT_Y_MIN)
        return center;
    /* 80181F48/50 initialize the large overlay font pools at bank+0x1428.
     * DEMONSTRATION is emitted there at y=32..48, above the usual menu band.
     * Keep its glyphs together. Timer/win-marker quads are separate earlier
     * allocations; glyph size alone cannot distinguish the win markers. */
    if (toshinden_is_battle_ui_packet(packet) &&
        role >= 0x1428u && role < 0x1428u + 0x140u * 0x28u)
        return center;
    if (packet_center < center - TOSHINDEN_CENTER_DEADZONE)
        return 0;
    if (packet_center > center + TOSHINDEN_CENTER_DEADZONE)
        return (int32_t)display_width;
    return center;
}

static int toshinden_should_tag_packet(uint32_t packet,
                                       int32_t min_y,
                                       int32_t max_y) {
    if (toshinden_is_battle_ui_packet(packet))
        return min_y >= TOSHINDEN_TOP_HUD_Y_MIN &&
               max_y <= TOSHINDEN_MENU_TEXT_Y_MAX;

    if (toshinden_is_pause_panel_packet(packet))
        return min_y >= TOSHINDEN_MENU_TEXT_Y_MIN &&
               max_y <= TOSHINDEN_MENU_TEXT_Y_MAX;

    if (toshinden_is_options_packet(packet))
        return min_y >= 0 && max_y <= 240;

    return 0;
}

static void toshinden_tag_packet_commands(uint32_t packet,
                                          uint32_t word_count,
                                          uint32_t display_width) {
    uint32_t word_index = 0;

    if ((!toshinden_is_battle_ui_packet(packet) &&
         !toshinden_is_pause_panel_packet(packet) &&
         !toshinden_is_options_packet(packet)) ||
        packet + (word_count + 1u) * 4u > TOSHINDEN_MAIN_RAM_END)
        return;

    while (word_index < word_count) {
        uint32_t command = packet + 4u + word_index * 4u;
        uint32_t command_packet = command - 4u;
        uint8_t op = psx_mod_read_byte(command + 3u);
        int command_words = toshinden_gp0_command_words(op);
        int32_t min_x;
        int32_t max_x;
        int32_t min_y;
        int32_t max_y;

        if (command_words <= 0)
            break;
        if (word_index + (uint32_t)command_words > word_count)
            break;

        /* The two health gauges deliberately grow with the viewport. Their
         * ten gradient quads include the fill, lost-health shade and border;
         * fit that entire group between the perimeter and the fixed-size
         * timer while text/icons retain their size. */
        if (toshinden_is_battle_ui_packet(command_packet)) {
            uint32_t role = (command_packet - TOSHINDEN_BATTLE_UI_BASE) %
                            TOSHINDEN_BATTLE_UI_STRIDE;
            if (op == 0x39u && role >= 0x78u && role < 0x1E0u) {
                if (role < 0x12Cu)
                    gpu_ws_tag_stretched_prim(command_packet, 32, 288, 0, 320);
                else
                    gpu_ws_tag_stretched_prim(command_packet, 352, 608, 320, 640);
                word_index += (uint32_t)command_words;
                continue;
            }
        }

        if (toshinden_packet_extent(command_packet, op,
                                    &min_x, &max_x, &min_y, &max_y) &&
            toshinden_should_tag_packet(command_packet, min_y, max_y)) {
            int32_t anchor = toshinden_anchor_for_packet(
                command_packet, min_x, max_x, min_y, display_width);
            if (toshinden_is_options_packet(command_packet))
                anchor = (int32_t)display_width / 2;
            gpu_ws_tag_screen_prim(command_packet, anchor);
        }

        word_index += (uint32_t)command_words;
    }
}

static void toshinden_final_ot_entry(CPUState *cpu, uint32_t address) {
    uint32_t head;
    uint32_t ot_base;
    uint32_t current;
    uint32_t display_width;
    uint32_t visited;

    if (!s_widescreen_enabled || address != TOSHINDEN_FINAL_OT_SUBMIT)
        return;
    if (psx_mod_widescreen_x_margin() <= 0)
        return;

    display_width = psx_mod_display_width();
    if (display_width == 0u)
        return;

    head = cpu->gpr[4];
    if (head != 0x801F25C0u && head != 0x801F4ED0u)
        return;

    ot_base = head - TOSHINDEN_OT_HEAD_DELTA;
    current = head;

    for (visited = 0; visited < TOSHINDEN_OT_WALK_LIMIT; ++visited) {
        uint32_t tag;
        uint32_t next;
        uint32_t word_count;

        if (!toshinden_is_main_ram(current))
            break;

        tag = psx_mod_read_word(current);
        next = tag & 0x00FFFFFFu;
        word_count = tag >> 24;

        if (current >= ot_base && current <= head &&
            ((current - ot_base) & 3u) == 0u) {
        } else if (word_count != 0u) {
            toshinden_tag_packet_commands(current, word_count, display_width);
        }

        if (next == TOSHINDEN_LINK_TERMINATOR)
            break;
        current = toshinden_link_to_ram(next);
        if (current == 0u)
            break;
    }
}

/*
 * Battle Arena Toshinden uses the squash widescreen path; native_wide=true
 * tears fighters and panorama strips. The activation callbacks select fixed or
 * adaptive aspect, and the final-OT hook tags proven battle HUD, pause font,
 * and pause-panel screen primitives after all scene/menu packets have been
 * linked. The renderer owns the actual transform, so guest packets and 4:3
 * behavior stay unchanged.
 */
static void toshinden_widescreen_16_9_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
    s_widescreen_enabled = 1;
}

static void toshinden_widescreen_adaptive_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
    (void)psx_mod_set_adaptive_display_aspect(32u, 9u);
    s_widescreen_enabled = 1;
}

PSX_MOD_CONSTRUCTOR(toshinden_register_widescreen_plugins) {
    (void)psx_mod_register_activation_plugin(
        "toshinden.widescreen.16-9", toshinden_widescreen_16_9_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.widescreen.adaptive", toshinden_widescreen_adaptive_activate);
    (void)psx_mod_register_function_entry_plugin(
        "toshinden.widescreen.final-ot", TOSHINDEN_FINAL_OT_SUBMIT,
        toshinden_final_ot_entry);
}
