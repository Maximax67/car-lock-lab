#include "motion_sensor.hpp"

void MotionSensor::init(GPIO_TypeDef *port, uint8_t pin,
                        SoftwareTimer *debounceTimer, uint32_t debounceMs,
                        ExtiPin::Trigger trigger) {
  m_debounceTimer = debounceTimer;
  m_debounceMs = debounceMs;

  // Floating input — the PIR drives the line actively; no pull needed.
  m_exti.init(port, pin, GpioInputMode::Floating, GpioPull::No, trigger);
  m_exti.setCallback(onExtiIrq, this);
}

void MotionSensor::setOnMotion(Callback cb, void *ctx) {
  m_callback = cb;
  m_ctx = ctx;
}

void MotionSensor::setOnMotionEnd(Callback cb, void *ctx) {
  m_endCallback = cb;
  m_endCtx = ctx;
}

// ── EXTI fires (rising or falling edge) ──────────────────────────────────────
// Mask the line and start the debounce one-shot.  Do NOT read the pin here
// or in onDebounce — a floating input has no defined idle level and the signal
// may have changed by the time we would read it.

void MotionSensor::onExtiIrq(void *ctx) {
  auto *self = static_cast<MotionSensor *>(ctx);
  self->m_exti.disable();
  self->m_debounceTimer->start(self->m_debounceMs, /*oneShot=*/true, onDebounce,
                               self);
}

// ── Debounce expired
// ────────────────────────────────────────────────────────── Re-enable the
// EXTI, then dispatch based on which edge we were expecting. Toggle the flag so
// the next call handles the opposite edge.

void MotionSensor::onDebounce(void *ctx) {
  auto *self = static_cast<MotionSensor *>(ctx);

  self->m_exti.enable();

  if (self->m_nextEdgeIsRising) {
    // Confirmed rising edge → motion started.
    self->m_nextEdgeIsRising = false;
    if (self->m_callback)
      self->m_callback(self->m_ctx);
  } else {
    // Confirmed falling edge → motion ended.
    self->m_nextEdgeIsRising = true;
    if (self->m_endCallback)
      self->m_endCallback(self->m_endCtx);
  }
}
