#ifndef TEST_STUB_GPU_H
#define TEST_STUB_GPU_H
#include <stdint.h>
void gpu_ws_tag_screen_prim(uint32_t packet, int32_t anchor);
void gpu_ws_tag_stretched_prim(uint32_t packet, int left_dst, int right_dst,
                               int left_src, int right_src);
const uint16_t *gpu_get_vram(void);
#endif
