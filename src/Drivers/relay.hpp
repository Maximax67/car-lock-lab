#pragma once

#include "HAL/gpio.hpp"
#include "HAL/Timer/software_timer.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Relay — drives a single relay coil via a GPIO output.
//
// Active HIGH: on() energises the coil.  Adjust if your relay board is
// active-LOW by swapping on()/off() calls inside the implementation.
//
// pulse() sets the relay ON for a configurable duration, then releases it
// automatically via a SoftwareTimer — no blocking.
// ─────────────────────────────────────────────────────────────────────────────
class Relay {
public:
  // port / pin  : GPIO coordinates.
  // timer       : a SoftwareTimer allocated from TimerManager::allocate().
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer);

  // Energise for durationMs then release automatically.
  // If a pulse is already in progress it is restarted.
  void pulse(uint32_t durationMs);

  // Manual control (bypasses timer).
  void on();
  void off();

  bool isOn() const;

private:
  static void onTimer(void *ctx);

  GpioPin m_pin;
  SoftwareTimer *m_timer = nullptr;
};
