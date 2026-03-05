#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include <cstdint>
#include <span>

// ─────────────────────────────────────────────────────────────────────────────
// FlashStep — one element of an LED flash sequence.
//   onMs  : LED on duration  (must be > 0)
//   offMs : LED off duration (0 = no gap between steps)
// ─────────────────────────────────────────────────────────────────────────────
struct FlashStep {
  uint32_t onMs;
  uint32_t offMs;
};

// ─────────────────────────────────────────────────────────────────────────────
// RgbLed — drives a common-cathode RGB LED via three independent GPIO pins,
// with one shared SoftwareTimer.
//
// Modes (mutually exclusive, last call wins):
//   Solid — constant colour, no timer activity.
//   Flash — plays a FlashStep array once (or looping); optional onDone
//           callback fires after the last step when repeat=false.
//   Blink — toggles at a configurable half-period.
//           setBlinkPeriod() updates speed without resetting phase:
//           stores the new value and the next self-scheduled tick picks it
//           up automatically — no stop/restart needed.
//
// Pattern lifetime: spans passed to playFlash() are stored by reference.
// The underlying arrays must outlive active playback.
// ─────────────────────────────────────────────────────────────────────────────
class RgbLed {
public:
  using Callback = void (*)(void *ctx) noexcept;

  void init(GPIO_TypeDef *rPort, uint8_t rPin, GPIO_TypeDef *gPort,
            uint8_t gPin, GPIO_TypeDef *bPort, uint8_t bPin,
            SoftwareTimer *timer) noexcept;

  // ── Solid colour ──────────────────────────────────────────────────────
  void setColor(bool r, bool g, bool b) noexcept;
  void off() noexcept;

  // ── Flash pattern ─────────────────────────────────────────────────────
  // Play steps with the given colour.  onDone fires when a non-repeating
  // pattern finishes.
  void playFlash(std::span<const FlashStep> steps, bool r, bool g, bool b,
                 bool repeat = false, Callback onDone = nullptr,
                 void *onDoneCtx = nullptr) noexcept;

  // ── Blink ─────────────────────────────────────────────────────────────
  // Start blinking at halfPeriodMs on / halfPeriodMs off.
  void startBlink(uint32_t halfPeriodMs, bool r, bool g, bool b) noexcept;

  // Change blink speed without resetting the phase.  No-op if not blinking.
  void setBlinkPeriod(uint32_t halfPeriodMs) noexcept;

  [[nodiscard]] bool isBlinking() const noexcept {
    return m_mode == Mode::Blink;
  }

private:
  enum class Mode { Idle, Flash, Blink };

  static void onTimer(void *ctx) noexcept;
  void setRaw(bool r, bool g, bool b) noexcept;
  void advanceFlashStep() noexcept;

  GpioPin m_r, m_g, m_b;
  SoftwareTimer *m_timer = nullptr;
  Mode m_mode = Mode::Idle;

  // Flash state
  std::span<const FlashStep> m_flashSteps{};
  uint8_t m_flashStep = 0;
  bool m_flashRepeat = false;
  bool m_flashInOn = false;
  bool m_flashR = false, m_flashG = false, m_flashB = false;
  Callback m_flashOnDone = nullptr;
  void *m_flashOnDoneCtx = nullptr;

  // Blink state
  uint32_t m_blinkHalfPeriod = 0;
  bool m_blinkState = false;
  bool m_blinkR = false, m_blinkG = false, m_blinkB = false;
};
