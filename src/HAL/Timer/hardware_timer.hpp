#pragma once

#include "stm32f103x6.h"
#include <cstdint>

class HardwareTimer {
public:
  using Callback = void (*)(void *ctx) noexcept;

  // timClkHz     : timer peripheral input clock in Hz
  // nvicPriority : NVIC pre-emption priority (0 = highest on Cortex-M3)
  void init(TIM_TypeDef *tim, uint32_t timClkHz,
            uint8_t nvicPriority = 0) noexcept;

  void start() noexcept;
  void stop() noexcept;

  void setCallback(Callback cb, void *ctx = nullptr) noexcept;

  // Call from the TIMx_IRQHandler — checks UIF, clears it, fires callback.
  void handleIrq() noexcept;

private:
  void enableClock() noexcept;
  [[nodiscard]] IRQn_Type irqnFor(TIM_TypeDef *tim) const noexcept;

  TIM_TypeDef *m_tim = nullptr;
  Callback m_cb = nullptr;
  void *m_ctx = nullptr;
};
