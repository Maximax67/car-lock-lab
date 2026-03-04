#include "relay.hpp"

void Relay::init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer) {
  m_pin.initAsOutput(port, pin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_pin.off();
  m_timer = timer;
}

void Relay::pulse(uint32_t durationMs) {
  m_pin.on();
  m_timer->start(durationMs, /*oneShot=*/true, onTimer, this);
}

void Relay::on() {
  m_timer->stop();
  m_pin.on();
}
void Relay::off() {
  m_timer->stop();
  m_pin.off();
}

bool Relay::isOn() const {
  // IDR bit reflects the output state for push-pull.
  return m_pin.read();
}

void Relay::onTimer(void *ctx) { static_cast<Relay *>(ctx)->m_pin.off(); }
