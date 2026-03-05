#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include <cstdint>
#include <span>

// ─────────────────────────────────────────────────────────────────────────────
// BeepPattern — one element of a buzzer sequence.
//   onMs  : how long the buzzer is ON  (must be > 0)
//   offMs : silent gap after this step (0 = no gap, advance immediately)
// ─────────────────────────────────────────────────────────────────────────────
struct BeepPattern {
  uint32_t onMs;
  uint32_t offMs;
};

// ─────────────────────────────────────────────────────────────────────────────
// Buzzer — drives an active (DC, on/off) buzzer connected to one GPIO pin.
//
// Call init() once with GPIO coordinates and a pre-allocated SoftwareTimer.
// Then use playPattern() for multi-step sequences or beep() for a simple
// single-duration tone.  All timing runs from the TIM2 ISR — no blocking.
//
// Pattern lifetime: the span passed to playPattern() is stored by reference.
// The underlying array must outlive the active playback (pass a constexpr /
// static array, not a local).
// ─────────────────────────────────────────────────────────────────────────────
class Buzzer {
public:
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer) noexcept;

  // Play a sequence of BeepPattern steps, optionally looping.
  void playPattern(std::span<const BeepPattern> pattern,
                   bool repeat = false) noexcept;

  // Convenience: single beep of the given duration.
  void beep(uint32_t durationMs) noexcept;

  // Stop any ongoing pattern and silence the buzzer immediately.
  void stop() noexcept;

  [[nodiscard]] bool isPlaying() const noexcept { return m_running; }

private:
  static void onTimer(void *ctx) noexcept;
  void advanceStep() noexcept;

  GpioPin m_pin;
  SoftwareTimer *m_timer = nullptr;

  std::span<const BeepPattern> m_pattern{};
  uint8_t m_step = 0;
  bool m_inOn = false;
  bool m_repeat = false;
  bool m_running = false;

  // Scratch storage for the single-beep helper.
  // Kept as a member so its lifetime covers the async timer callback.
  BeepPattern m_singleBeep{};
};
