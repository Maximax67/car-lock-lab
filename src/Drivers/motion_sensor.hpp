#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/exti.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// MotionSensor — interrupt-driven wrapper for a PIR or digital motion sensor,
// with hardware debounce identical in structure to Button.
//
// Edge-direction detection without reading the pin
// ─────────────────────────────────────────────────
// The GPIO is configured as a floating input (PIR drives the line actively).
// A floating pin with no pull resistor has no defined idle level, so reading
// IDR after the debounce window is unreliable — the level may have changed or
// may simply read LOW regardless of the real signal.
//
// Instead, edge direction is tracked in software:
//   m_nextEdgeIsRising starts true (sensor idle = LOW, first event = rising).
//   After each confirmed event the flag is toggled, so we always know which
//   edge just fired without touching the pin at all.
//
//   rising edge confirmed  → fire m_callback    (motion start)
//                            m_nextEdgeIsRising = false
//   falling edge confirmed → fire m_endCallback (motion end)
//                            m_nextEdgeIsRising = true
//
// Wiring assumption: sensor output HIGH = motion active (standard PIR).
// ─────────────────────────────────────────────────────────────────────────────
class MotionSensor {
public:
  using Callback = void (*)(void *ctx);

  // port / pin      : GPIO coordinates of the sensor output.
  // debounceTimer   : SoftwareTimer from TimerManager::allocate().
  // debounceMs      : glitch-rejection window (default: 50 ms).
  // trigger         : must be Both to detect motion start and end.
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
            uint32_t debounceMs = 50,
            ExtiPin::Trigger trigger = ExtiPin::Trigger::Both);

  // Fires on every confirmed motion-start edge (sensor output goes HIGH).
  void setOnMotion(Callback cb, void *ctx = nullptr);

  // Fires on every confirmed motion-end edge (sensor output goes LOW).
  // Optional: if not set, falling edges are silently ignored.
  void setOnMotionEnd(Callback cb, void *ctx = nullptr);

  bool isActive() const { return m_exti.read(); }

  void enable() { m_exti.enable(); }
  void disable() { m_exti.disable(); }

private:
  static void onExtiIrq(void *ctx); // raw edge → start debounce (no pin read)
  static void
  onDebounce(void *ctx); // debounce expired → dispatch by tracked edge

  ExtiPin m_exti;
  SoftwareTimer *m_debounceTimer = nullptr;
  uint32_t m_debounceMs = 50;

  // Tracks which edge is expected next.  Toggled after every confirmed event.
  // Starts true: sensor idle = LOW, so the first real event is a rising edge.
  bool m_nextEdgeIsRising = true;

  Callback m_callback = nullptr; // motion-start (rising edge)
  void *m_ctx = nullptr;
  Callback m_endCallback = nullptr; // motion-end (falling edge)
  void *m_endCtx = nullptr;
};
