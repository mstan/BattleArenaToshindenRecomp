#include "cpu_state.h"
#include "mod_plugins.h"

#include <stdint.h>

#define TOSHINDEN_BOSS_UI_FINAL_OT       0x80197C70u
#define TOSHINDEN_SELECT_SCENE           0x801CA584u
#define TOSHINDEN_SELECT_STATE_OFFSET    0x154u
#define TOSHINDEN_PLAYER_1               0x801BC114u
#define TOSHINDEN_PLAYER_2               0x801BC1E8u
#define TOSHINDEN_PLAYER_MODE_OFFSET     0x02u
#define TOSHINDEN_PLAYER_CHAR_OFFSET     0x06u

#define TOSHINDEN_CHAR_GAIA              8
#define TOSHINDEN_CHAR_SHO               9

#define TOSHINDEN_OT_HEAD_0              0x801F25C0u
#define TOSHINDEN_OT_HEAD_1              0x801F4ED0u

#define TOSHINDEN_UI_DMA_BUFFER_SIZE     (32u * 1024u)
#define TOSHINDEN_UI_MAX_OT_WALK         8192u
#define TOSHINDEN_UI_LINK_END            0x00FFFFFFu

#define TOSHINDEN_SCREEN_W               640
#define TOSHINDEN_SCREEN_H               240

#define TOSHINDEN_CARD_X                 286
#define TOSHINDEN_CARD_W                 68
#define TOSHINDEN_CARD_H                 34
#define TOSHINDEN_GAIA_Y                 68
#define TOSHINDEN_SHO_Y                  120

#define TOSHINDEN_FONT_ADVANCE           12
#define TOSHINDEN_FONT_SCALE_X           2
#define TOSHINDEN_FONT_HEIGHT            7

#define TOSHINDEN_NATIVE_SELECT_STRIDE   0x2910u
#define TOSHINDEN_NATIVE_CURSOR_0        0x801F2B44u
#define TOSHINDEN_NATIVE_CURSOR_1        0x801F2B54u
#define TOSHINDEN_NATIVE_LABEL_0_A       0x801F2A2Cu
#define TOSHINDEN_NATIVE_LABEL_0_B       0x801F2A54u
#define TOSHINDEN_NATIVE_LABEL_1_A       0x801F2A7Cu
#define TOSHINDEN_NATIVE_LABEL_1_B       0x801F2AA4u
#define TOSHINDEN_NATIVE_COM_LABEL_A     0x801F2ACCu
#define TOSHINDEN_NATIVE_COM_LABEL_B     0x801F2AF4u
#define TOSHINDEN_NATIVE_VS_LOGO         0x801F2B1Cu
#define TOSHINDEN_NATIVE_HILITE_0_BASE   0x801F28ECu
#define TOSHINDEN_NATIVE_HILITE_STRIDE   0x28u
#define TOSHINDEN_NATIVE_THUMBNAIL_COUNT 8u
#define TOSHINDEN_NATIVE_THUMBNAIL_Y     190u
#define TOSHINDEN_NATIVE_THUMBNAIL_BOT_Y 224u

#define TOSHINDEN_GAIA_TPAGE             0x010Au
#define TOSHINDEN_SHO_TPAGE              0x010Cu
#define TOSHINDEN_BOSS_PORTRAIT_CLUT     0x0000u
#define TOSHINDEN_BOSS_PORTRAIT_U0       0u
#define TOSHINDEN_BOSS_PORTRAIT_V0       128u
#define TOSHINDEN_BOSS_PORTRAIT_U1       127u
#define TOSHINDEN_BOSS_PORTRAIT_V1       255u

static int s_boss_ui_enabled;
static uint32_t s_dma_buffers[2];
static uint32_t s_saved_hilite_command[2][8];
static uint8_t s_hilite_saved[2][8];

extern void toshinden_boss_portraits_prepare(void);

typedef struct ToshindenUiBuilder {
    uint32_t base;
    uint32_t cursor;
    uint32_t end;
    uint32_t first;
    uint32_t previous;
    uint32_t previous_word_count;
} ToshindenUiBuilder;

typedef struct ToshindenBossPortraitDescriptor {
    uint16_t u0;
    uint16_t v0;
    uint16_t u1;
    uint16_t v1;
    uint16_t u2;
    uint16_t v2;
    uint16_t u3;
    uint16_t v3;
    uint16_t clut;
    uint16_t tpage;
} ToshindenBossPortraitDescriptor;

static uint32_t toshinden_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16);
}

static uint32_t toshinden_xy(int32_t x, int32_t y) {
    return ((uint32_t)(uint16_t)y << 16) | (uint32_t)(uint16_t)x;
}

static uint32_t toshinden_wh(int32_t w, int32_t h) {
    return ((uint32_t)(uint16_t)h << 16) | (uint32_t)(uint16_t)w;
}

static int toshinden_is_boss_char(int32_t char_id) {
    return char_id == TOSHINDEN_CHAR_GAIA || char_id == TOSHINDEN_CHAR_SHO;
}

static int toshinden_is_normal_char(int32_t char_id) {
    return char_id >= 0 && char_id <= 7;
}

static int toshinden_wrapped_normal_char(int32_t char_id) {
    return char_id & 7;
}

static int toshinden_player_char(uint32_t player) {
    return (int16_t)psx_mod_read_half(player + TOSHINDEN_PLAYER_CHAR_OFFSET);
}

static int toshinden_player_is_cpu(uint32_t player) {
    return (psx_mod_read_half(player + TOSHINDEN_PLAYER_MODE_OFFSET) & 1u) != 0u;
}

static int toshinden_select_is_active(void) {
    int32_t state = (int16_t)psx_mod_read_half(
        TOSHINDEN_SELECT_SCENE + TOSHINDEN_SELECT_STATE_OFFSET);

    return state >= 5 && state <= 13;
}

static int toshinden_ot_bank(uint32_t head) {
    if (head == TOSHINDEN_OT_HEAD_0)
        return 0;
    if (head == TOSHINDEN_OT_HEAD_1)
        return 1;
    return -1;
}

static int toshinden_address_in_buffer(uint32_t address, uint32_t base) {
    return address >= base &&
        address < (base + TOSHINDEN_UI_DMA_BUFFER_SIZE);
}

static int toshinden_find_ot_tail(uint32_t head, uint32_t *out_tail) {
    uint32_t node = head;
    uint32_t i;

    for (i = 0; i < TOSHINDEN_UI_MAX_OT_WALK; i++) {
        uint32_t tag = psx_mod_read_word(node);
        uint32_t next = tag & TOSHINDEN_UI_LINK_END;
        uint32_t b;

        for (b = 0; b < 2; b++) {
            if (s_dma_buffers[b] != 0u &&
                toshinden_address_in_buffer(node, s_dma_buffers[b]))
                return 0;
        }

        if (next == TOSHINDEN_UI_LINK_END) {
            *out_tail = node;
            return 1;
        }

        if ((next & 3u) != 0u)
            return 0;

        node = 0x80000000u | next;
    }

    return 0;
}

static int toshinden_is_native_thumbnail_packet(uint32_t packet) {
    uint32_t command = psx_mod_read_word(packet + 4u);
    uint32_t xy0 = psx_mod_read_word(packet + 8u);
    uint32_t xy2 = psx_mod_read_word(packet + 24u);
    uint32_t op = command >> 24;
    uint32_t y0 = (xy0 >> 16) & 0xFFFFu;
    uint32_t y2 = (xy2 >> 16) & 0xFFFFu;

    if (op != 0x2Cu && op != 0x2Du)
        return 0;

    return y0 == TOSHINDEN_NATIVE_THUMBNAIL_Y &&
        y2 == TOSHINDEN_NATIVE_THUMBNAIL_BOT_Y;
}

static int toshinden_ot_contains_select_thumbnails(uint32_t head, int bank) {
    uint32_t bank_offset = bank == 0 ? 0u : TOSHINDEN_NATIVE_SELECT_STRIDE;
    uint32_t node = head;
    uint32_t found = 0;
    uint32_t i;

    for (i = 0; i < TOSHINDEN_UI_MAX_OT_WALK; i++) {
        uint32_t tag = psx_mod_read_word(node);
        uint32_t next = tag & TOSHINDEN_UI_LINK_END;
        uint32_t thumb;

        if (next == TOSHINDEN_UI_LINK_END)
            return found != 0u;

        if ((next & 3u) != 0u)
            return 0;

        for (thumb = 0; thumb < TOSHINDEN_NATIVE_THUMBNAIL_COUNT; thumb++) {
            uint32_t packet = TOSHINDEN_NATIVE_HILITE_0_BASE + bank_offset +
                (thumb * TOSHINDEN_NATIVE_HILITE_STRIDE);

            if (next == (packet & TOSHINDEN_UI_LINK_END) &&
                toshinden_is_native_thumbnail_packet(packet))
                found |= 1u << thumb;
        }

        node = 0x80000000u | next;
    }

    return 0;
}

static int toshinden_ot_contains_packet(uint32_t head, uint32_t target) {
    uint32_t target_next = target & TOSHINDEN_UI_LINK_END;
    uint32_t node = head;
    uint32_t i;

    for (i = 0; i < TOSHINDEN_UI_MAX_OT_WALK; i++) {
        uint32_t tag = psx_mod_read_word(node);
        uint32_t next = tag & TOSHINDEN_UI_LINK_END;

        if (next == TOSHINDEN_UI_LINK_END)
            return 0;

        if ((next & 3u) != 0u)
            return 0;

        if (next == target_next)
            return 1;

        node = 0x80000000u | next;
    }

    return 0;
}

static void toshinden_unlink_ot_packet(uint32_t head, uint32_t target) {
    uint32_t target_next = target & TOSHINDEN_UI_LINK_END;
    uint32_t node = head;
    uint32_t i;

    for (i = 0; i < TOSHINDEN_UI_MAX_OT_WALK; i++) {
        uint32_t tag = psx_mod_read_word(node);
        uint32_t next = tag & TOSHINDEN_UI_LINK_END;

        if (next == TOSHINDEN_UI_LINK_END)
            return;

        if ((next & 3u) != 0u)
            return;

        if (next == target_next) {
            uint32_t target_tag = psx_mod_read_word(target);
            uint32_t after = target_tag & TOSHINDEN_UI_LINK_END;

            psx_mod_write_word(node, (tag & 0xFF000000u) | after);
            if (after == TOSHINDEN_UI_LINK_END || (after & 3u) != 0u)
                return;
            continue;
        }

        node = 0x80000000u | next;
    }
}

static int toshinden_is_native_vs_logo_packet(uint32_t packet) {
    uint32_t command = psx_mod_read_word(packet + 4u);
    uint32_t xy0 = psx_mod_read_word(packet + 8u);
    uint32_t xy1 = psx_mod_read_word(packet + 16u);
    uint32_t xy2 = psx_mod_read_word(packet + 24u);
    uint32_t xy3 = psx_mod_read_word(packet + 32u);

    return command == 0x2C808080u &&
        xy0 == toshinden_xy(256, 98) &&
        xy1 == toshinden_xy(384, 98) &&
        xy2 == toshinden_xy(256, 162) &&
        xy3 == toshinden_xy(384, 162);
}

static void toshinden_unlink_native_vs_logo(uint32_t head, int bank) {
    uint32_t packet = TOSHINDEN_NATIVE_VS_LOGO +
        (bank == 0 ? 0u : TOSHINDEN_NATIVE_SELECT_STRIDE);

    if (toshinden_is_native_vs_logo_packet(packet))
        toshinden_unlink_ot_packet(head, packet);
}

static int toshinden_is_native_com_label_packet(uint32_t packet) {
    uint32_t command = psx_mod_read_word(packet + 4u);
    uint32_t xy0 = psx_mod_read_word(packet + 8u);
    uint32_t xy1 = psx_mod_read_word(packet + 16u);
    uint32_t xy2 = psx_mod_read_word(packet + 24u);
    uint32_t xy3 = psx_mod_read_word(packet + 32u);
    uint32_t y0 = (xy0 >> 16) & 0xFFFFu;
    uint32_t y1 = (xy1 >> 16) & 0xFFFFu;
    uint32_t y2 = (xy2 >> 16) & 0xFFFFu;
    uint32_t y3 = (xy3 >> 16) & 0xFFFFu;
    uint32_t x0 = xy0 & 0xFFFFu;
    uint32_t x1 = xy1 & 0xFFFFu;
    uint32_t x2 = xy2 & 0xFFFFu;
    uint32_t x3 = xy3 & 0xFFFFu;

    return command == 0x2CFFFFFFu &&
        x0 == x2 && x1 == x3 && (x1 - x0) == 17u &&
        y0 == 178u && y1 == 178u && y2 == 187u && y3 == 187u;
}

static void toshinden_unlink_native_com_label(uint32_t head, int bank) {
    uint32_t bank_offset = bank == 0 ? 0u : TOSHINDEN_NATIVE_SELECT_STRIDE;
    uint32_t packet_a = TOSHINDEN_NATIVE_COM_LABEL_A + bank_offset;
    uint32_t packet_b = TOSHINDEN_NATIVE_COM_LABEL_B + bank_offset;

    if (toshinden_is_native_com_label_packet(packet_a))
        toshinden_unlink_ot_packet(head, packet_a);
    if (toshinden_is_native_com_label_packet(packet_b))
        toshinden_unlink_ot_packet(head, packet_b);
}

static void toshinden_save_hilite_command(int bank, uint32_t slot,
                                          uint32_t packet) {
    if (!s_hilite_saved[bank][slot]) {
        s_saved_hilite_command[bank][slot] = psx_mod_read_word(packet + 4u);
        s_hilite_saved[bank][slot] = 1;
    }
}

static void toshinden_restore_hilite_command(int bank, uint32_t slot,
                                             uint32_t packet) {
    if (s_hilite_saved[bank][slot]) {
        psx_mod_write_word(packet + 4u, s_saved_hilite_command[bank][slot]);
        s_hilite_saved[bank][slot] = 0;
    }
}

static void toshinden_dim_native_hilite(uint32_t packet, int bank,
                                        uint32_t slot) {
    toshinden_save_hilite_command(bank, slot, packet);
    psx_mod_write_word(packet + 4u, 0x2C646464u);
}

static void toshinden_suppress_native_boss_cursor(uint32_t head, int bank,
                                                  int p1_char, int p2_char) {
    uint32_t bank_offset = bank == 0 ? 0u : TOSHINDEN_NATIVE_SELECT_STRIDE;
    int p1_boss = toshinden_is_boss_char(p1_char);
    int p2_boss = toshinden_is_boss_char(p2_char);

    if (!p1_boss && !p2_boss)
        return;

    if (p1_boss) {
        toshinden_unlink_ot_packet(head, TOSHINDEN_NATIVE_CURSOR_0 + bank_offset);
        toshinden_unlink_ot_packet(head, TOSHINDEN_NATIVE_LABEL_0_A + bank_offset);
        toshinden_unlink_ot_packet(head, TOSHINDEN_NATIVE_LABEL_0_B + bank_offset);
    }

    if (p2_boss) {
        toshinden_unlink_ot_packet(head, TOSHINDEN_NATIVE_CURSOR_1 + bank_offset);
        toshinden_unlink_ot_packet(head, TOSHINDEN_NATIVE_LABEL_1_A + bank_offset);
        toshinden_unlink_ot_packet(head, TOSHINDEN_NATIVE_LABEL_1_B + bank_offset);
        if (toshinden_player_is_cpu(TOSHINDEN_PLAYER_2))
            toshinden_unlink_native_com_label(head, bank);
    }

    if (p1_boss && !(toshinden_is_normal_char(p2_char) &&
        toshinden_wrapped_normal_char(p2_char) ==
        toshinden_wrapped_normal_char(p1_char))) {
        uint32_t slot = (uint32_t)toshinden_wrapped_normal_char(p1_char);
        toshinden_dim_native_hilite(
            TOSHINDEN_NATIVE_HILITE_0_BASE + bank_offset +
            (slot * TOSHINDEN_NATIVE_HILITE_STRIDE), bank, slot);
    }

    if (p2_boss && !(toshinden_is_normal_char(p1_char) &&
        toshinden_wrapped_normal_char(p1_char) ==
        toshinden_wrapped_normal_char(p2_char))) {
        uint32_t slot = (uint32_t)toshinden_wrapped_normal_char(p2_char);
        toshinden_dim_native_hilite(
            TOSHINDEN_NATIVE_HILITE_0_BASE + bank_offset +
            (slot * TOSHINDEN_NATIVE_HILITE_STRIDE), bank, slot);
    }
}

static void toshinden_restore_native_hilites(int bank) {
    uint32_t bank_offset = bank == 0 ? 0u : TOSHINDEN_NATIVE_SELECT_STRIDE;
    uint32_t slot;

    for (slot = 0; slot < TOSHINDEN_NATIVE_THUMBNAIL_COUNT; slot++) {
        toshinden_restore_hilite_command(bank, slot,
            TOSHINDEN_NATIVE_HILITE_0_BASE + bank_offset +
            (slot * TOSHINDEN_NATIVE_HILITE_STRIDE));
    }
}

static void toshinden_builder_init(ToshindenUiBuilder *builder,
                                   uint32_t base) {
    builder->base = base;
    builder->cursor = base;
    builder->end = base + TOSHINDEN_UI_DMA_BUFFER_SIZE;
    builder->first = 0;
    builder->previous = 0;
    builder->previous_word_count = 0;
}

static int toshinden_emit_words(ToshindenUiBuilder *builder,
                                const uint32_t *words,
                                uint32_t word_count) {
    uint32_t packet;
    uint32_t i;

    if (word_count == 0u || word_count > 255u)
        return 0;

    if ((builder->cursor + ((word_count + 1u) * 4u)) > builder->end)
        return 0;

    packet = builder->cursor;
    builder->cursor += (word_count + 1u) * 4u;

    if (builder->first == 0u)
        builder->first = packet;

    if (builder->previous != 0u) {
        psx_mod_write_word(builder->previous,
            (builder->previous_word_count << 24) |
            (packet & TOSHINDEN_UI_LINK_END));
    }

    builder->previous = packet;
    builder->previous_word_count = word_count;

    for (i = 0; i < word_count; i++)
        psx_mod_write_word(packet + 4u + (i * 4u), words[i]);

    return 1;
}

static void toshinden_emit_rect(ToshindenUiBuilder *builder,
                                int32_t x, int32_t y,
                                int32_t w, int32_t h,
                                uint32_t color) {
    uint32_t words[3];

    if (w <= 0 || h <= 0)
        return;

    if (x >= TOSHINDEN_SCREEN_W || y >= TOSHINDEN_SCREEN_H)
        return;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if ((x + w) > TOSHINDEN_SCREEN_W)
        w = TOSHINDEN_SCREEN_W - x;
    if ((y + h) > TOSHINDEN_SCREEN_H)
        h = TOSHINDEN_SCREEN_H - y;

    if (w <= 0 || h <= 0)
        return;

    words[0] = 0x60000000u | color;
    words[1] = toshinden_xy(x, y);
    words[2] = toshinden_wh(w, h);
    (void)toshinden_emit_words(builder, words, 3u);
}

static void toshinden_emit_ft4(ToshindenUiBuilder *builder,
                               int32_t x, int32_t y,
                               int32_t w, int32_t h,
                               const ToshindenBossPortraitDescriptor *desc) {
    uint32_t words[9];

    if (w <= 0 || h <= 0)
        return;

    words[0] = 0x2D000000u;
    words[1] = toshinden_xy(x, y);
    words[2] = ((uint32_t)desc->clut << 16) |
        ((uint32_t)desc->v0 << 8) | (uint32_t)desc->u0;
    words[3] = toshinden_xy(x + w, y);
    words[4] = ((uint32_t)desc->tpage << 16) |
        ((uint32_t)desc->v1 << 8) | (uint32_t)desc->u1;
    words[5] = toshinden_xy(x, y + h);
    words[6] = ((uint32_t)desc->v2 << 8) | (uint32_t)desc->u2;
    words[7] = toshinden_xy(x + w, y + h);
    words[8] = ((uint32_t)desc->v3 << 8) | (uint32_t)desc->u3;

    (void)toshinden_emit_words(builder, words, 9u);
}

static void toshinden_emit_border(ToshindenUiBuilder *builder,
                                  int32_t x, int32_t y,
                                  int32_t w, int32_t h,
                                  int32_t inset,
                                  uint32_t color) {
    x += inset;
    y += inset;
    w -= inset * 2;
    h -= inset * 2;

    if (w <= 1 || h <= 1)
        return;

    toshinden_emit_rect(builder, x, y, w, 1, color);
    toshinden_emit_rect(builder, x, y + h - 1, w, 1, color);
    toshinden_emit_rect(builder, x, y, 1, h, color);
    toshinden_emit_rect(builder, x + w - 1, y, 1, h, color);
}

static uint8_t toshinden_font_rows(char c, int row) {
    static const uint8_t digits[10][7] = {
        { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },
        { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },
        { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },
        { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E },
        { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },
        { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E },
        { 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E },
        { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },
        { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },
        { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E },
    };

    if (row < 0 || row >= TOSHINDEN_FONT_HEIGHT)
        return 0;

    if (c >= '0' && c <= '9')
        return digits[c - '0'][row];

    switch (c) {
    case 'A': { static const uint8_t r[7] = { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return r[row]; }
    case 'B': { static const uint8_t r[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }; return r[row]; }
    case 'C': { static const uint8_t r[7] = { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }; return r[row]; }
    case 'D': { static const uint8_t r[7] = { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }; return r[row]; }
    case 'E': { static const uint8_t r[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }; return r[row]; }
    case 'F': { static const uint8_t r[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }; return r[row]; }
    case 'G': { static const uint8_t r[7] = { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F }; return r[row]; }
    case 'H': { static const uint8_t r[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return r[row]; }
    case 'I': { static const uint8_t r[7] = { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }; return r[row]; }
    case 'L': { static const uint8_t r[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }; return r[row]; }
    case 'M': { static const uint8_t r[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }; return r[row]; }
    case 'N': { static const uint8_t r[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }; return r[row]; }
    case 'O': { static const uint8_t r[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return r[row]; }
    case 'P': { static const uint8_t r[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }; return r[row]; }
    case 'R': { static const uint8_t r[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }; return r[row]; }
    case 'S': { static const uint8_t r[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }; return r[row]; }
    case 'T': { static const uint8_t r[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return r[row]; }
    case 'U': { static const uint8_t r[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return r[row]; }
    case 'W': { static const uint8_t r[7] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A }; return r[row]; }
    case 'X': { static const uint8_t r[7] = { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }; return r[row]; }
    case '/': { static const uint8_t r[7] = { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 }; return r[row]; }
    case ':': { static const uint8_t r[7] = { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 }; return r[row]; }
    default:
        return 0;
    }
}

static void toshinden_emit_text(ToshindenUiBuilder *builder,
                                int32_t x, int32_t y,
                                const char *text,
                                uint32_t color) {
    int32_t pen_x = x;

    while (*text != '\0') {
        char c = *text++;
        int32_t row;

        if (c == ' ') {
            pen_x += TOSHINDEN_FONT_ADVANCE;
            continue;
        }

        for (row = 0; row < TOSHINDEN_FONT_HEIGHT; row++) {
            uint8_t bits = toshinden_font_rows(c, row);
            int32_t bit = 0;

            while (bit < 5) {
                int32_t run_start;
                int32_t run_len;

                while (bit < 5 && ((bits & (uint8_t)(1u << (4 - bit))) == 0u))
                    bit++;

                if (bit >= 5)
                    break;

                run_start = bit;
                run_len = 0;
                while (bit < 5 && ((bits & (uint8_t)(1u << (4 - bit))) != 0u)) {
                    bit++;
                    run_len++;
                }

                toshinden_emit_rect(builder,
                    pen_x + (run_start * TOSHINDEN_FONT_SCALE_X),
                    y + row,
                    run_len * TOSHINDEN_FONT_SCALE_X,
                    1,
                    color);
            }
        }

        pen_x += TOSHINDEN_FONT_ADVANCE;
    }
}

static int toshinden_get_boss_portrait(int char_id,
                                       ToshindenBossPortraitDescriptor *out) {
    if (char_id != TOSHINDEN_CHAR_GAIA && char_id != TOSHINDEN_CHAR_SHO)
        return 0;

    out->u0 = TOSHINDEN_BOSS_PORTRAIT_U0;
    out->v0 = TOSHINDEN_BOSS_PORTRAIT_V0;
    out->u1 = TOSHINDEN_BOSS_PORTRAIT_U1;
    out->v1 = TOSHINDEN_BOSS_PORTRAIT_V0;
    out->u2 = TOSHINDEN_BOSS_PORTRAIT_U0;
    out->v2 = TOSHINDEN_BOSS_PORTRAIT_V1;
    out->u3 = TOSHINDEN_BOSS_PORTRAIT_U1;
    out->v3 = TOSHINDEN_BOSS_PORTRAIT_V1;
    out->clut = TOSHINDEN_BOSS_PORTRAIT_CLUT;
    out->tpage = char_id == TOSHINDEN_CHAR_GAIA ?
        TOSHINDEN_GAIA_TPAGE : TOSHINDEN_SHO_TPAGE;
    return 1;
}

static void toshinden_emit_portrait_if_configured(ToshindenUiBuilder *builder,
                                                  int char_id,
                                                  int32_t x, int32_t y,
                                                  int32_t w, int32_t h) {
    ToshindenBossPortraitDescriptor desc;

    if (!toshinden_get_boss_portrait(char_id, &desc))
        return;

    toshinden_emit_ft4(builder, x, y, w, h, &desc);
}

static void toshinden_emit_big_portrait(ToshindenUiBuilder *builder,
                                        int char_id,
                                        int player_index) {
    int32_t x = player_index == 0 ? 22 : 362;
    int32_t y = 34;
    int32_t w = 256;
    int32_t h = 128;

    if (!toshinden_is_boss_char(char_id))
        return;

    toshinden_emit_rect(builder, x, y, w, h, toshinden_rgb(0, 0, 0));
    toshinden_emit_portrait_if_configured(builder, char_id, x, y, w, h);
}

static void toshinden_emit_player_badge(ToshindenUiBuilder *builder,
                                        int player_index,
                                        int32_t x, int32_t y,
                                        uint32_t color) {
    if (player_index == 0) {
        toshinden_emit_rect(builder, x, y, 26, 10, toshinden_rgb(36, 8, 8));
        toshinden_emit_text(builder, x + 2, y + 2, "1P", color);
    } else {
        if (toshinden_player_is_cpu(TOSHINDEN_PLAYER_2)) {
            toshinden_emit_rect(builder, x, y, 38, 10, toshinden_rgb(8, 16, 40));
            toshinden_emit_text(builder, x + 2, y + 2, "COM", color);
        } else {
            toshinden_emit_rect(builder, x, y, 26, 10, toshinden_rgb(8, 16, 40));
            toshinden_emit_text(builder, x + 2, y + 2, "2P", color);
        }
    }
}

static void toshinden_emit_boss_card(ToshindenUiBuilder *builder,
                                     int char_id,
                                     int32_t y,
                                     const char *name,
                                     int p1_selected,
                                     int p2_selected) {
    uint32_t dim = toshinden_rgb(24, 24, 32);
    uint32_t panel = toshinden_rgb(0, 0, 0);
    uint32_t border = toshinden_rgb(136, 136, 152);
    uint32_t white = toshinden_rgb(224, 224, 224);
    uint32_t p1 = toshinden_rgb(248, 64, 48);
    uint32_t p2 = toshinden_rgb(80, 128, 255);
    int32_t name_x = char_id == TOSHINDEN_CHAR_GAIA ? 296 : 302;

    toshinden_emit_rect(builder, TOSHINDEN_CARD_X - 2, y - 2,
        TOSHINDEN_CARD_W + 4, TOSHINDEN_CARD_H + 4, dim);
    toshinden_emit_rect(builder, TOSHINDEN_CARD_X, y,
        TOSHINDEN_CARD_W, TOSHINDEN_CARD_H, panel);
    toshinden_emit_portrait_if_configured(builder, char_id,
        TOSHINDEN_CARD_X, y, TOSHINDEN_CARD_W, TOSHINDEN_CARD_H);
    toshinden_emit_border(builder, TOSHINDEN_CARD_X, y,
        TOSHINDEN_CARD_W, TOSHINDEN_CARD_H, 0, border);

    if (p1_selected)
        toshinden_emit_border(builder, TOSHINDEN_CARD_X, y,
            TOSHINDEN_CARD_W, TOSHINDEN_CARD_H, 2, p1);
    if (p2_selected)
        toshinden_emit_border(builder, TOSHINDEN_CARD_X, y,
            TOSHINDEN_CARD_W, TOSHINDEN_CARD_H, p1_selected ? 5 : 2, p2);

    if (p1_selected)
        toshinden_emit_player_badge(builder, 0,
            TOSHINDEN_CARD_X + 3, y + 3, p1);
    if (p2_selected)
        toshinden_emit_player_badge(builder, 1,
            TOSHINDEN_CARD_X + TOSHINDEN_CARD_W -
            (toshinden_player_is_cpu(TOSHINDEN_PLAYER_2) ? 41 : 29),
            y + 3, p2);

    toshinden_emit_text(builder, name_x, y + TOSHINDEN_CARD_H + 3,
        name, white);
}

static void toshinden_build_boss_ui(ToshindenUiBuilder *builder) {
    int p1_char = toshinden_player_char(TOSHINDEN_PLAYER_1);
    int p2_char = toshinden_player_char(TOSHINDEN_PLAYER_2);
    uint32_t heading = toshinden_rgb(248, 220, 96);
    uint32_t shadow = toshinden_rgb(0, 0, 0);

    toshinden_emit_big_portrait(builder, p1_char, 0);
    toshinden_emit_big_portrait(builder, p2_char, 1);

    toshinden_emit_rect(builder, 282, 50, 76, 16, shadow);
    toshinden_emit_text(builder, 290, 54, "EXTRA", heading);

    toshinden_emit_boss_card(builder, TOSHINDEN_CHAR_GAIA,
        TOSHINDEN_GAIA_Y, "GAIA",
        p1_char == TOSHINDEN_CHAR_GAIA,
        p2_char == TOSHINDEN_CHAR_GAIA);
    toshinden_emit_boss_card(builder, TOSHINDEN_CHAR_SHO,
        TOSHINDEN_SHO_Y, "SHO",
        p1_char == TOSHINDEN_CHAR_SHO,
        p2_char == TOSHINDEN_CHAR_SHO);

}

static void toshinden_finish_builder(ToshindenUiBuilder *builder) {
    if (builder->previous != 0u)
        psx_mod_write_word(builder->previous,
            (builder->previous_word_count << 24) | TOSHINDEN_UI_LINK_END);
}

static int toshinden_ensure_dma_buffers(void) {
    uint32_t i;

    for (i = 0; i < 2; i++) {
        if (s_dma_buffers[i] == 0u) {
            s_dma_buffers[i] = psx_mod_alloc_gpu_dma_memory(
                TOSHINDEN_UI_DMA_BUFFER_SIZE, 4u);
            if (s_dma_buffers[i] == 0u)
                return 0;
        }
    }

    return 1;
}

static void toshinden_append_ui_chain(uint32_t head,
                                      uint32_t first_packet) {
    uint32_t tail;
    uint32_t tail_tag;

    if (first_packet == 0u)
        return;

    if (!toshinden_find_ot_tail(head, &tail))
        return;

    tail_tag = psx_mod_read_word(tail);
    psx_mod_write_word(tail,
        (tail_tag & 0xFF000000u) | (first_packet & TOSHINDEN_UI_LINK_END));
}

static void toshinden_boss_roster_ui_final_ot(CPUState *cpu,
                                              uint32_t address) {
    uint32_t head;
    int bank;
    int p1_char;
    int p2_char;
    ToshindenUiBuilder builder;

    if (!s_boss_ui_enabled || address != TOSHINDEN_BOSS_UI_FINAL_OT)
        return;

    head = cpu->gpr[4];
    bank = toshinden_ot_bank(head);
    if (bank < 0)
        return;

    if (!toshinden_select_is_active())
        return;

    if (!toshinden_ot_contains_select_thumbnails(head, bank))
        return;

    if (!toshinden_ensure_dma_buffers())
        return;

    p1_char = toshinden_player_char(TOSHINDEN_PLAYER_1);
    p2_char = toshinden_player_char(TOSHINDEN_PLAYER_2);
    toshinden_boss_portraits_prepare();
    toshinden_restore_native_hilites(bank);
    toshinden_unlink_native_vs_logo(head, bank);
    toshinden_suppress_native_boss_cursor(head, bank, p1_char, p2_char);

    toshinden_builder_init(&builder, s_dma_buffers[bank]);
    toshinden_build_boss_ui(&builder);
    toshinden_finish_builder(&builder);
    toshinden_append_ui_chain(head, builder.first);
}

static void toshinden_boss_roster_ui_activate(void) {
    s_boss_ui_enabled = 1;
}

PSX_MOD_CONSTRUCTOR(toshinden_register_boss_roster_ui_plugins) {
    (void)psx_mod_register_activation_plugin(
        "toshinden.boss-roster.ui", toshinden_boss_roster_ui_activate);
    (void)psx_mod_register_function_entry_plugin(
        "toshinden.boss-roster.ui-final-ot", TOSHINDEN_BOSS_UI_FINAL_OT,
        toshinden_boss_roster_ui_final_ot);
}
