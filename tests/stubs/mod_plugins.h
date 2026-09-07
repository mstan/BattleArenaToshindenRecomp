#ifndef TEST_STUB_MOD_PLUGINS_H
#define TEST_STUB_MOD_PLUGINS_H
#include <stdint.h>
struct CPUState;
typedef void (*PSXModActivationCallback)(void);
typedef void (*PSXModVBlankCallback)(void);
typedef void (*PSXModFunctionEntryCallback)(struct CPUState *cpu, uint32_t address);
int psx_mod_register_activation_plugin(const char *id, PSXModActivationCallback callback);
int psx_mod_register_vblank_plugin(const char *id, PSXModVBlankCallback callback);
int psx_mod_register_function_entry_plugin(const char *id, uint32_t address, PSXModFunctionEntryCallback callback);
void psx_mod_function_entry(struct CPUState *cpu, uint32_t address);
int psx_mod_game_started(void);
uint8_t psx_mod_read_byte(uint32_t address);
void psx_mod_write_byte(uint32_t address, uint8_t value);
uint16_t psx_mod_read_half(uint32_t address);
void psx_mod_write_half(uint32_t address, uint16_t value);
uint32_t psx_mod_read_word(uint32_t address);
void psx_mod_write_word(uint32_t address, uint32_t value);
void psx_mod_write_code_word(uint32_t address, uint32_t value);
uint32_t psx_mod_alloc_guest_memory(uint32_t size, uint32_t alignment);
uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t size, uint32_t alignment);
int32_t psx_mod_widescreen_x_margin(void);
uint32_t psx_mod_display_width(void);
uint32_t psx_mod_display_height(void);
int psx_mod_option_value(const char *package_id, const char *feature_id,
                         const char *option_id, char *out, uint32_t out_size);
int psx_mod_current_resource_path(const char *resource_id, char *out, uint32_t out_size);
int psx_mod_set_fixed_display_aspect(uint32_t numerator, uint32_t denominator);
int psx_mod_set_adaptive_display_aspect(uint32_t max_numerator, uint32_t denominator);
int psx_mod_set_native_vblank_rate(uint32_t frames_per_second);
int psx_mod_set_frame_interpolation(uint32_t frames_per_second);
int psx_mod_set_frame_interpolation_blend(uint32_t blend_mode);
int psx_mod_set_auto_skip_fmv(int enabled);
int psx_mod_set_bezel_artwork(const char *path);
int psx_mod_set_load_acceleration(uint32_t wall_clock_multiplier, uint32_t read_speed_multiplier);
int psx_mod_set_disc_speed(uint32_t divisor, uint32_t seek_divisor);
#define PSX_MOD_CONSTRUCTOR(name) static void name(void)
#endif
