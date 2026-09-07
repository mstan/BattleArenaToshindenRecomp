#include "mod_plugins.h"

/*
 * Battle Arena Toshinden's guest cadence stays stock. These callbacks only
 * select how often PSXrecomp's OpenGL presenter blends completed game frames.
 */
static void toshinden_frame_rate_set(unsigned frames_per_second) {
    (void)psx_mod_set_frame_interpolation_blend(
        PSX_MOD_FRAME_INTERPOLATION_MOTION_ADAPTIVE);
    (void)psx_mod_set_frame_interpolation(frames_per_second);
}

static void toshinden_frame_rate_display_activate(void) {
    /* Zero follows the measured display refresh; it must never busy-loop. */
    toshinden_frame_rate_set(0u);
}

static void toshinden_frame_rate_60_activate(void) {
    toshinden_frame_rate_set(60u);
}

static void toshinden_frame_rate_90_activate(void) {
    toshinden_frame_rate_set(90u);
}

static void toshinden_frame_rate_120_activate(void) {
    toshinden_frame_rate_set(120u);
}

static void toshinden_frame_rate_144_activate(void) {
    toshinden_frame_rate_set(144u);
}

static void toshinden_frame_rate_165_activate(void) {
    toshinden_frame_rate_set(165u);
}

static void toshinden_frame_rate_240_activate(void) {
    toshinden_frame_rate_set(240u);
}

PSX_MOD_CONSTRUCTOR(toshinden_register_frame_rate_plugins) {
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.display", toshinden_frame_rate_display_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.60", toshinden_frame_rate_60_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.90", toshinden_frame_rate_90_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.120", toshinden_frame_rate_120_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.144", toshinden_frame_rate_144_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.165", toshinden_frame_rate_165_activate);
    (void)psx_mod_register_activation_plugin(
        "toshinden.framerate.240", toshinden_frame_rate_240_activate);
}
