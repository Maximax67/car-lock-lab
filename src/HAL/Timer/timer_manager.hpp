#pragma once

#include "hardware_timer.hpp"
#include "software_timer.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// TimerManager — singleton.
//
// Owns one HardwareTimer (TIM2 by default) configured for a 1 ms interrupt,
// and a fixed pool of SoftwareTimers that are ticked on every hardware
// interrupt.
//
// Callers obtain a SoftwareTimer* from allocate() and pass it to driver init()
// functions. Allocated timers are never returned to the pool — the pool is
// sized generously so this is fine for a static system like a car alarm.
//
// Zero heap: everything lives in BSS / stack.
// ─────────────────────────────────────────────────────────────────────────────
constexpr uint8_t MAX_SW_TIMERS = 20;

class TimerManager {
public:
  static TimerManager &instance();

  // Call once from main() before constructing any drivers.
  // timClkHz: TIMx peripheral input clock (see HardwareTimer note).
  void init(TIM_TypeDef *tim, uint32_t timClkHz, uint8_t nvicPriority = 0);

  // Returns a pointer to an unoccupied SoftwareTimer slot.
  // Halts in an infinite loop if the pool is exhausted (debug assertion).
  SoftwareTimer *allocate();

  // Must be called from the TIMx_IRQHandler
  void handleTimerIrq();

  uint32_t ticks() const { return m_ticks; }

private:
  TimerManager() = default;
  TimerManager(const TimerManager &) = delete;
  TimerManager &operator=(const TimerManager &) = delete;

  static void onHwTick(void *ctx);

  HardwareTimer m_hwTimer;
  SoftwareTimer m_pool[MAX_SW_TIMERS];
  bool m_used[MAX_SW_TIMERS] = {};
  volatile uint32_t m_ticks = 0;
};
