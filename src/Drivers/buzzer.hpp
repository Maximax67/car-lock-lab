#pragma once

#include "HAL/gpio.hpp"
#include "HAL/Timer/software_timer.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// BeepPattern — one element of a buzzer sequence.
//   onMs  : how long the buzzer is ON for this step  (0 = skip step)
//   offMs : silent gap after this step               (0 = no gap)
// ─────────────────────────────────────────────────────────────────────────────
struct BeepPattern {
  uint32_t onMs;
  uint32_t offMs;
};

// ─────────────────────────────────────────────────────────────────────────────
// Buzzer — drives an active buzzer (DC, on/off) connected to one GPIO pin.
//
// Call init() once with the GPIO coordinates and a pre-allocated SoftwareTimer.
// Then call playPattern() or playAlarm() / stop() as needed.
//
// Internals: the timer fires after each phase (on or off), advances through
// the pattern array, and optionally loops.  Everything runs from the TIM2 ISR
// context — no blocking anywhere.
// ─────────────────────────────────────────────────────────────────────────────
class Buzzer {
public:
  // port / pin   : GPIO coordinates of the buzzer signal
  // timer        : a SoftwareTimer allocated from TimerManager::allocate()
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer);

  // Play a fixed pattern once (or repeated if repeat=true).
  // pattern / count : pointer + length of a BeepPattern array.
  void playPattern(const BeepPattern *pattern, uint8_t count,
                   bool repeat = false);

  // Convenience: play a single beep of given duration.
  void beep(uint32_t durationMs);

  // Stop any ongoing pattern and silence the buzzer.
  void stop();

  bool isPlaying() const { return m_running; }

private:
  static void onTimer(void *ctx);
  void advanceStep();

  GpioPin m_pin;
  SoftwareTimer *m_timer = nullptr;

  const BeepPattern *m_pattern = nullptr;
  uint8_t m_count = 0;
  uint8_t m_step = 0;
  bool m_inOn = false;
  bool m_repeat = false;
  bool m_running = false;

  // Scratch storage for the single-beep convenience helper.
  BeepPattern m_singleBeep{};
};
