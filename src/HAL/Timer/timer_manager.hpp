#pragma once

#include "hardware_timer.hpp"
#include "software_timer.hpp"
#include <array>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// TimerManager — singleton owner of the hardware tick and all software timers.
//
// Owns one HardwareTimer (TIM2 by default) configured for a 1 ms interrupt,
// and a fixed pool of SoftwareTimers that are ticked on every hardware
// interrupt.
//
// Usage
// ─────
// • Call init() once from main() before constructing any drivers.
// • Obtain SoftwareTimer slots with allocate() and pass them to driver init().
// • Allocated timers are never returned — the pool is sized for the whole
//   system lifetime and the allocation pattern is fully static.
//
// Timer budget (13 of MAX_SW_TIMERS = 20 used):
//   Buttons  ×3  : 2 each (debounce + double-click window) = 6
//   Motion sensor: 1 (debounce)
//   Relays   ×2  : 1 each                                  = 2
//   Buzzer       : 1
//   RGB LED      : 1
//   CarAlarm     : 2 (pre-alarm countdown + blink interpolator)
//   ──────────────────────────────────────────────────────────
//   Total        : 13
//
// Zero heap: everything lives in BSS / data.
// ─────────────────────────────────────────────────────────────────────────────

constexpr uint8_t MAX_SW_TIMERS = 20;

class TimerManager {
public:
  [[nodiscard]] static TimerManager &instance() noexcept;

  // Call once from main() before constructing any drivers.
  void init(TIM_TypeDef *tim, uint32_t timClkHz,
            uint8_t nvicPriority = 0) noexcept;

  // Returns a pointer to a free SoftwareTimer slot.
  // Calls halPanic() if the pool is exhausted — indicates a budget overrun.
  [[nodiscard]] SoftwareTimer *allocate() noexcept;

  // Must be called from the TIMx_IRQHandler.
  void handleTimerIrq() noexcept;

  [[nodiscard]] uint32_t ticks() const noexcept { return m_ticks; }

private:
  TimerManager() = default;
  TimerManager(const TimerManager &) = delete;
  TimerManager &operator=(const TimerManager &) = delete;

  static void onHwTick(void *ctx) noexcept;

  HardwareTimer m_hwTimer;
  std::array<SoftwareTimer, MAX_SW_TIMERS> m_pool{};
  std::array<bool, MAX_SW_TIMERS> m_used{};
  volatile uint32_t m_ticks = 0;
};
