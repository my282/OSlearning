#include <stdint.h>

extern "C" void __attribute__((ms_abi)) KernelMain(uint64_t frame_buffer_base,
                                                   uint64_t frame_buffer_size) {
  uint32_t *frame_buffer = reinterpret_cast<uint32_t *>(frame_buffer_base);
  // ++iの方が早いらしい
  for (uint64_t i = 0; i < frame_buffer_size / 4; ++i) {
    frame_buffer[i] = 0x0021a6cc;
  }
  while (1) {
    __asm__("hlt");
  }
};
