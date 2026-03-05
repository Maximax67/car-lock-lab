#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Relay — drives a single relay coil via a GPIO output.
//
// Active HIGH: on() energises the coil.  Swap on()/off() in the
// implementation if your relay board is active-LOW.
//
// pulse() energises the relay for a configurable duration then releases it
// automatically via a SoftwareTimer — no blocking anywhere.
// ─────────────────────────────────────────────────────────────────────────────
class Relay {
public:
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer) noexcept;

  // Energise for durationMs then release automatically.
  // If a pulse is already in progress it is extended from now.
  void pulse(uint32_t durationMs) noexcept;

  // Manual control — bypasses the timer.
  void on() noexcept;
  void off() noexcept;

  // True when the output is currently HIGH (IDR reflects ODR for push-pull).
  [[nodiscard]] bool isOn() const noexcept;

private:
  static void onTimer(void *ctx) noexcept;

  GpioPin m_pin;
  SoftwareTimer *m_timer = nullptr;
};
