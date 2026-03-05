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

// ── EXTI fires (rising edge = motion edge)
// ──────────────────────────────────── Mask the line immediately so any glitch
// during the debounce window is ignored, then start the debounce one-shot.

void MotionSensor::onExtiIrq(void *ctx) {
  auto *self = static_cast<MotionSensor *>(ctx);

  // Suppress further edges during the debounce window.
  self->m_exti.disable();

  // Start (or restart) the debounce one-shot.
  self->m_debounceTimer->start(self->m_debounceMs, /*oneShot=*/true, onDebounce,
                               self);
}

// ── Debounce expired
// ────────────────────────────────────────────────────────── Re-enable the EXTI
// and fire the callback unconditionally.
//
// IMPORTANT: Do NOT re-read the pin here.
//
// The rising edge that fired onExtiIrq is the confirmation of motion.
// Re-reading the pin after 50 ms would silently reject any signal shorter
// than the debounce window: a brief test pulse, a fast PIR output, or any
// signal that returns low before the timer fires — all would be discarded.
//
// The debounce window serves one purpose only: prevent rapid re-triggering
// by masking subsequent edges for 50 ms.  The motion event is guaranteed
// by the edge itself, not by the pin level afterwards.

void MotionSensor::onDebounce(void *ctx) {
  auto *self = static_cast<MotionSensor *>(ctx);

  // Re-arm before firing so the next edge is never missed.
  self->m_exti.enable();

  // Always fire — the rising edge already confirmed motion.
  if (self->m_callback)
    self->m_callback(self->m_ctx);
}
