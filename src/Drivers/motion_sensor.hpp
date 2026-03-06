#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/exti.hpp"
#include <cstdint>

class MotionSensor {
public:
  using Callback = void (*)(void *ctx) noexcept;

  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
            uint32_t debounceMs = 50,
            ExtiPin::Trigger trigger = ExtiPin::Trigger::Both) noexcept;

  void setOnMotion(Callback cb, void *ctx = nullptr) noexcept;
  void setOnMotionEnd(Callback cb, void *ctx = nullptr) noexcept;

  [[nodiscard]] bool isActive() const noexcept { return m_exti.read(); }

  void enable() noexcept { m_exti.enable(); }
  void disable() noexcept { m_exti.disable(); }

private:
  static void onExtiIrq(void *ctx) noexcept;
  static void onDebounce(void *ctx) noexcept;

  ExtiPin m_exti;
  SoftwareTimer *m_debounceTimer = nullptr;
  uint32_t m_debounceMs = 50;

  bool m_lastStableState = false;
  bool m_pendingState = false;

  Callback m_onMotion = nullptr;
  void *m_onMotionCtx = nullptr;
  Callback m_onMotionEnd = nullptr;
  void *m_onMotionEndCtx = nullptr;
};
