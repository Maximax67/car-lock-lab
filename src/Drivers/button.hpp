#pragma once

#include "HAL/exti.hpp"
#include "HAL/Timer/software_timer.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Button — interrupt-driven push-button with:
//   • Hardware debounce (EXTI disabled during debounce window).
//   • Single-click callback (fires when double-click window expires with 1
//   hit). • Double-click callback (fires immediately on second confirmed
//   click).
//
// Wiring assumption (recommended for stability):
//   External pull-up → pin is HIGH at rest, pulled LOW by the button.
//   ↳ GpioPull::Up  +  Trigger::Falling  is used internally.
//
// Timer requirements: two pre-allocated SoftwareTimers per Button instance.
//   debounceTimer   : BUTTON_DEBOUNCE_MS one-shot (30 ms default).
//   windowTimer     : BUTTON_DOUBLE_CLICK_MS one-shot (300 ms default).
//
// ISR-safe: all shared state updated with IRQs disabled where necessary.
// ─────────────────────────────────────────────────────────────────────────────
class Button {
public:
  using Callback = void (*)(void *ctx);

  // port / pin         : GPIO coordinates.
  // debounceTimer      : SoftwareTimer from TimerManager::allocate().
  // windowTimer        : SoftwareTimer from TimerManager::allocate().
  // debounceMs         : override debounce window (default: 30 ms).
  // doubleClickWindowMs: override double-click window (default: 300 ms).
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
            SoftwareTimer *windowTimer, uint32_t debounceMs = 30,
            uint32_t doubleClickWindowMs = 300);

  void setOnSingleClick(Callback cb, void *ctx = nullptr);
  void setOnDoubleClick(Callback cb, void *ctx = nullptr);

  // Read instantaneous pin state: true = pressed (pin LOW with pull-up).
  bool isPressed() const { return !m_exti.read(); }

private:
  // Static trampoline callbacks for SoftwareTimer and ExtiPin.
  static void onExtiIrq(void *ctx);
  static void onDebounce(void *ctx);
  static void onWindow(void *ctx);

  ExtiPin m_exti;
  SoftwareTimer *m_debounceTimer = nullptr;
  SoftwareTimer *m_windowTimer = nullptr;

  uint32_t m_debounceMs = 30;
  uint32_t m_windowMs = 300;

  volatile uint8_t m_pendingClicks = 0;

  Callback m_singleClickCb = nullptr;
  void *m_singleClickCtx = nullptr;
  Callback m_doubleClickCb = nullptr;
  void *m_doubleClickCtx = nullptr;
};
