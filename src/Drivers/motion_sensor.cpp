#include "motion_sensor.hpp"

void MotionSensor::init(GPIO_TypeDef *port, uint8_t pin,
                        SoftwareTimer *debounceTimer, uint32_t debounceMs,
                        ExtiPin::Trigger trigger) noexcept {
  m_debounceTimer = debounceTimer;
  m_debounceMs = debounceMs;

  // Floating input — the PIR drives the line actively; no pull needed.
  m_exti.init(port, pin, GpioInputMode::Floating, GpioPull::No, trigger);
  m_nextEdgeIsRising = !m_exti.read();
  m_exti.setCallback(onExtiIrq, this);
}

void MotionSensor::setOnMotion(Callback cb, void *ctx) noexcept {
  __disable_irq();
  m_onMotion = cb;
  m_onMotionCtx = ctx;
  __enable_irq();
}

void MotionSensor::setOnMotionEnd(Callback cb, void *ctx) noexcept {
  __disable_irq();
  m_onMotionEnd = cb;
  m_onMotionEndCtx = ctx;
  __enable_irq();
}

void MotionSensor::onExtiIrq(void *ctx) noexcept {
  auto *self = static_cast<MotionSensor *>(ctx);
  self->m_exti.disable();
  self->m_debounceTimer->start(self->m_debounceMs, /*oneShot=*/true, onDebounce,
                               self);
}

void MotionSensor::onDebounce(void *ctx) noexcept {
  auto *self = static_cast<MotionSensor *>(ctx);
  self->m_exti.enable();

  if (self->m_nextEdgeIsRising) {
    self->m_nextEdgeIsRising = false;
    if (self->m_onMotion) {
      self->m_onMotion(self->m_onMotionCtx);
    }
  } else {
    self->m_nextEdgeIsRising = true;
    if (self->m_onMotionEnd) {
      self->m_onMotionEnd(self->m_onMotionEndCtx);
    }
  }
}
