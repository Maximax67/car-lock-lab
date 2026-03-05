#include "relay.hpp"

void Relay::init(GPIO_TypeDef *port, uint8_t pin,
                 SoftwareTimer *timer) noexcept {
  m_pin.initAsOutput(port, pin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_pin.off();
  m_timer = timer;
}

void Relay::pulse(uint32_t durationMs) noexcept {
  m_pin.on();
  m_timer->start(durationMs, /*oneShot=*/true, onTimer, this);
}

void Relay::on() noexcept {
  m_timer->stop();
  m_pin.on();
}

void Relay::off() noexcept {
  m_timer->stop();
  m_pin.off();
}

[[nodiscard]] bool Relay::isOn() const noexcept { return m_pin.read(); }

void Relay::onTimer(void *ctx) noexcept {
  static_cast<Relay *>(ctx)->m_pin.off();
}
