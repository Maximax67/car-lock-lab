#pragma once

#include "stm32f103x6.h"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// HardwareTimer — thin wrapper around a TIMx peripheral.
//
// Configured exclusively for a 1 ms update-event interrupt, which feeds
// TimerManager. Nothing else should use this class directly.
// ─────────────────────────────────────────────────────────────────────────────
class HardwareTimer {
public:
  using Callback = void (*)(void *ctx);

  // timClkHz   : timer peripheral input clock in Hz (see note above)
  // nvicPriority: NVIC pre-emption priority (0 = highest on Cortex-M3)
  void init(TIM_TypeDef *tim, uint32_t timClkHz, uint8_t nvicPriority = 0);

  void start();
  void stop();

  void setCallback(Callback cb, void *ctx = nullptr);

  // Call from the TIMx_IRQHandler — checks UIF, clears it, fires callback
  void handleIrq();

private:
  void enableClock();
  IRQn_Type irqnFor(TIM_TypeDef *tim) const;

  TIM_TypeDef *m_tim = nullptr;
  Callback m_cb = nullptr;
  void *m_ctx = nullptr;
};
