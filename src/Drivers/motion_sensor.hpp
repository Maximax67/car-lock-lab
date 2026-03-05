#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/exti.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// MotionSensor — interrupt-driven wrapper for a PIR / digital motion sensor,
// with hardware debounce identical in structure to Button.
//
// Edge-direction tracking without reading the pin
// ────────────────────────────────────────────────
// The GPIO is configured as floating (the PIR drives the line actively; no
// pull resistor is needed or appropriate).  A floating pin with no pull has
// no defined idle level — reading IDR after the debounce window is unreliable.
//
// Instead, direction is tracked in software:
//   m_nextEdgeIsRising = true  on construction (sensor idle = LOW).
//   After each confirmed event the flag is toggled so we always know which
//   edge just fired without touching the pin at all.
//
//   Rising  edge confirmed → fire m_onMotion    (motion start)
//                          → m_nextEdgeIsRising = false
//   Falling edge confirmed → fire m_onMotionEnd (motion end)
//                          → m_nextEdgeIsRising = true
//
// Wiring assumption: sensor output HIGH = motion active (standard PIR).
// ─────────────────────────────────────────────────────────────────────────────
class MotionSensor {
public:
  using Callback = void (*)(void *ctx) noexcept;

  // debounceMs : glitch-rejection window (default: 50 ms).
  // trigger    : must be Both to detect motion start and end.
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
            uint32_t debounceMs = 50,
            ExtiPin::Trigger trigger = ExtiPin::Trigger::Both) noexcept;

  // Fires on every confirmed motion-start edge (sensor HIGH).
  void setOnMotion(Callback cb, void *ctx = nullptr) noexcept;

  // Fires on every confirmed motion-end edge (sensor LOW).
  // Optional: if not set, falling edges are silently ignored.
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

  // Tracks which edge is expected next; toggled after every confirmed event.
  // Starts true: sensor idle = LOW, so the first real event is a rising edge.
  bool m_nextEdgeIsRising = true;

  Callback m_onMotion = nullptr;
  void *m_onMotionCtx = nullptr;
  Callback m_onMotionEnd = nullptr;
  void *m_onMotionEndCtx = nullptr;
};
