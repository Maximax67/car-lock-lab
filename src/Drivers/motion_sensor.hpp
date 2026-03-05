#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/exti.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// MotionSensor — interrupt-driven wrapper for a PIR or digital motion sensor,
// with hardware debounce identical in structure to Button.
//
// Why debounce?  PIR outputs and manually-driven test signals can produce
// multiple rapid edges (glitches) at the start of detection.  Without
// debounce, the first edge enters PreAlarm and a glitch edge immediately
// escalates to FullAlarm before the LED even starts blinking.
//
// Debounce logic (mirrors Button):
//   1. Rising edge fires → EXTI masked, debounce one-shot started.
//   2. Debounce expires → EXTI re-armed, pin read:
//        HIGH → genuine motion event → callback fired.
//        LOW  → glitch / noise        → silently discarded.
//
// Wiring assumption: sensor output HIGH = motion active (standard PIR).
// ─────────────────────────────────────────────────────────────────────────────
class MotionSensor {
public:
  using Callback = void (*)(void *ctx);

  // port / pin      : GPIO coordinates of the sensor output.
  // debounceTimer   : SoftwareTimer from TimerManager::allocate().
  // debounceMs      : glitch-rejection window (default: 50 ms).
  // trigger         : edge to interrupt on (default: Rising = motion start).
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
            uint32_t debounceMs = 50,
            ExtiPin::Trigger trigger = ExtiPin::Trigger::Rising);

  // Register a callback that fires on every confirmed motion edge.
  void setOnMotion(Callback cb, void *ctx = nullptr);

  // Read current sensor output: true = motion currently active.
  bool isActive() const { return m_exti.read(); }

  // Called by CarAlarm to arm / disarm the sensor.
  void enable() { m_exti.enable(); }
  void disable() { m_exti.disable(); }

private:
  static void onExtiIrq(void *ctx);  // raw edge → start debounce
  static void onDebounce(void *ctx); // debounce expired → validate + fire

  ExtiPin m_exti;
  SoftwareTimer *m_debounceTimer = nullptr;
  uint32_t m_debounceMs = 50;

  Callback m_callback = nullptr;
  void *m_ctx = nullptr;
};
