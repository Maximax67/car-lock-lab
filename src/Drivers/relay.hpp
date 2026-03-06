#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include <cstdint>

class Relay {
public:
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer) noexcept;

  // If a pulse is already in progress it is extended from now.
  void pulse(uint32_t durationMs) noexcept;
  void on() noexcept;
  void off() noexcept;

  [[nodiscard]] bool isOn() const noexcept;

private:
  static void onTimer(void *ctx) noexcept;

  GpioPin m_pin;
  SoftwareTimer *m_timer = nullptr;
};
