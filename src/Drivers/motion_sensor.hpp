#pragma once

#include "HAL/exti.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// MotionSensor — interrupt-driven wrapper for a PIR or digital motion sensor.
//
// The sensor output is assumed to be HIGH when motion is detected and LOW
// otherwise (standard PIR active-HIGH output).  The EXTI fires on a Rising
// edge (motion start).  Adjust the trigger in init() if your hardware differs.
//
// Usage:
//   MotionSensor ms;
//   ms.init(GPIOA, 7);
//   ms.setOnMotion([](void* ctx){ ... }, myCtx);
// ─────────────────────────────────────────────────────────────────────────────
class MotionSensor {
public:
  using Callback = void (*)(void *ctx);

  // port / pin : GPIO coordinates of the sensor output.
  // trigger    : edge to interrupt on (default: Rising = motion start).
  void init(GPIO_TypeDef *port, uint8_t pin,
            ExtiPin::Trigger trigger = ExtiPin::Trigger::Rising);

  // Register a callback that fires on every detected motion edge.
  void setOnMotion(Callback cb, void *ctx = nullptr);

  // Read current sensor output: true = motion currently active.
  bool isActive() const { return m_exti.read(); }

  void enable() { m_exti.enable(); }
  void disable() { m_exti.disable(); }

private:
  static void onExtiIrq(void *ctx);

  ExtiPin m_exti;
  Callback m_callback = nullptr;
  void *m_ctx = nullptr;
};
