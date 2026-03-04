#include "motion_sensor.hpp"

void MotionSensor::init(GPIO_TypeDef *port, uint8_t pin,
                        ExtiPin::Trigger trigger) {
  // Floating input — the PIR drives the line actively; no pull needed.
  m_exti.init(port, pin, GpioInputMode::Floating, GpioPull::No, trigger);
  m_exti.setCallback(onExtiIrq, this);
}

void MotionSensor::setOnMotion(Callback cb, void *ctx) {
  m_callback = cb;
  m_ctx = ctx;
}

void MotionSensor::onExtiIrq(void *ctx) {
  auto *self = static_cast<MotionSensor *>(ctx);
  if (self->m_callback)
    self->m_callback(self->m_ctx);
}
