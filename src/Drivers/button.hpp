#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/exti.hpp"
#include <cstdint>

class Button {
public:
  using Callback = void (*)(void *ctx) noexcept;

  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
            SoftwareTimer *windowTimer, uint32_t debounceMs = 30,
            uint32_t doubleClickWindowMs = 300) noexcept;

  void setOnSingleClick(Callback cb, void *ctx = nullptr) noexcept;
  void setOnDoubleClick(Callback cb, void *ctx = nullptr) noexcept;

  // True when the pin is currently LOW (button pressed).
  [[nodiscard]] bool isPressed() const noexcept { return !m_exti.read(); }

private:
  static void onExtiIrq(void *ctx) noexcept;
  static void onDebounce(void *ctx) noexcept;
  static void onWindow(void *ctx) noexcept;

  ExtiPin m_exti;
  SoftwareTimer *m_debounceTimer = nullptr;
  SoftwareTimer *m_windowTimer = nullptr;

  uint32_t m_debounceMs = 30;
  uint32_t m_windowMs = 300;

  // Written inside TIM2 ISR (onDebounce / onWindow) — declared volatile so
  // the compiler does not cache the value across ISR boundaries.
  volatile uint8_t m_pendingClicks = 0;

  Callback m_singleClickCb = nullptr;
  void *m_singleClickCtx = nullptr;
  Callback m_doubleClickCb = nullptr;
  void *m_doubleClickCtx = nullptr;
};
