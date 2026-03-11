#pragma once

#include "stm32f103x6.h"
#include <cstdint>

class SysTickTimer {
public:
  using Callback = void (*)(void *ctx) noexcept;

  void init(uint32_t coreClkHz, uint8_t nvicPriority = 0) noexcept;
  void setCallback(Callback cb, void *ctx = nullptr) noexcept;
  void handleIrq() noexcept;

private:
  Callback m_cb = nullptr;
  void *m_ctx = nullptr;
};
