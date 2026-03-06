#include "motion_sensor.hpp"

void MotionSensor::init(GPIO_TypeDef *port, uint8_t pin,
                        SoftwareTimer *debounceTimer, uint32_t debounceMs,
                        ExtiPin::Trigger trigger) noexcept {
  m_debounceTimer = debounceTimer;
  m_debounceMs = debounceMs;

  m_exti.init(port, pin, GpioInputMode::PullUpDown, GpioPull::Down, trigger);
  __DSB(); // ensure GPIO config is fully committed before reading IDR
  m_lastStableState = m_exti.read();
  m_pendingState = m_lastStableState;

  m_exti.setCallback(onExtiIrq, this);
}

void MotionSensor::setOnMotion(Callback cb, void *ctx) noexcept {
  __disable_irq();
  m_onMotion = cb;
  m_onMotionCtx = ctx;
  __enable_irq();

  // If the sensor was already active at init() time,
  // no rising edge will ever fire.
  if (m_lastStableState) {
    cb(ctx);
  }
}

void MotionSensor::setOnMotionEnd(Callback cb, void *ctx) noexcept {
  __disable_irq();
  m_onMotionEnd = cb;
  m_onMotionEndCtx = ctx;
  __enable_irq();
}

void MotionSensor::onExtiIrq(void *ctx) noexcept {
  auto *self = static_cast<MotionSensor *>(ctx);
  self->m_pendingState = self->m_exti.read();
  self->m_exti.disable();
  self->m_debounceTimer->start(self->m_debounceMs, /*oneShot=*/true, onDebounce,
                               self);
}

void MotionSensor::onDebounce(void *ctx) noexcept {
  auto *self = static_cast<MotionSensor *>(ctx);
  const bool currentState = self->m_exti.read();

  if (currentState != self->m_pendingState ||
      currentState == self->m_lastStableState) {
    self->m_exti.enable();
    return;
  }

  self->m_lastStableState = currentState;
  self->m_exti.enable();

  if (currentState) {
    if (self->m_onMotion) {
      self->m_onMotion(self->m_onMotionCtx);
    }
  } else {
    if (self->m_onMotionEnd) {
      self->m_onMotionEnd(self->m_onMotionEndCtx);
    }
  }
}
