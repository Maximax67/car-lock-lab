#pragma once

#include "stm32f103x6.h"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// HardwareTimer — thin wrapper around a TIMx peripheral.
//
// Configured exclusively for a 1 ms update-event interrupt, which feeds
// TimerManager. Nothing else should use this class directly.
//
// Clock note for STM32F103 at 72 MHz (standard PLL config):
//   SYSCLK = 72 MHz, APB1 prescaler = /2 → PCLK1 = 36 MHz
//   TIM2-7 input clock = 2 × PCLK1 = 72 MHz  (when APB1 prescaler ≠ 1)
//   TIM1   input clock = PCLK2     = 72 MHz
//   → pass timClkHz = 72'000'000 for any timer on this board.
//
// Timer register strategy for 1 ms period at 72 MHz:
//   PSC  = 7199  → timer clock = 72 MHz / 7200 = 10 kHz  (0.1 ms per tick)
//   ARR  = 9     → interrupt every 10 ticks = 1 ms
//   Both values fit in 16-bit registers. ✓
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
