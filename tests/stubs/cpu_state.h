#ifndef TEST_STUB_CPU_STATE_H
#define TEST_STUB_CPU_STATE_H
#include <stdint.h>
typedef struct CPUState {
    uint32_t gpr[32];
} CPUState;
#endif
