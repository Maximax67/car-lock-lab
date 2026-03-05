#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// FlashStep — one element of an LED flash sequence.
//   onMs  : LED on duration   (0 = skip)
//   offMs : LED off duration  (0 = no gap)
// ─────────────────────────────────────────────────────────────────────────────
struct FlashStep {
  uint32_t onMs;
  uint32_t offMs;
};

// ─────────────────────────────────────────────────────────────────────────────
// RgbLed — drives a common-cathode RGB LED (3 separate GPIO pins, one per
// channel).  All three channels share a single SoftwareTimer.
//
// Modes (mutually exclusive):
//   Solid   — constant colour, no timer activity.
//   Flash   — plays a FlashStep array once (or repeating), then stops.
//             onDone callback fires after the last step when repeat=false.
//   Blink   — toggles between colour and off at a configurable half-period.
//             setBlinkPeriod() updates the speed without restarting from zero.
// ─────────────────────────────────────────────────────────────────────────────
class RgbLed {
public:
  using Callback = void (*)(void *ctx);

  // rPort/rPin, gPort/gPin, bPort/bPin : one GPIO per channel.
  // timer : a SoftwareTimer allocated from TimerManager::allocate().
  void init(GPIO_TypeDef *rPort, uint8_t rPin, GPIO_TypeDef *gPort,
            uint8_t gPin, GPIO_TypeDef *bPort, uint8_t bPin,
            SoftwareTimer *timer);

  // ── Solid colour ─────────────────────────────────────────────────────
  // Stops any running pattern and sets the LED to a fixed colour.
  void setColor(bool r, bool g, bool b);

  // Turn off all channels and stop any pattern.
  void off();

  // ── Flash pattern ─────────────────────────────────────────────────────
  // Play an array of FlashStep with the given colour, optionally repeating.
  // onDone / onDoneCtx : optional callback fired when a non-repeating flash
  //                      finishes (not called for repeat=true patterns).
  void playFlash(const FlashStep *steps, uint8_t count, bool r, bool g, bool b,
                 bool repeat = false, Callback onDone = nullptr,
                 void *onDoneCtx = nullptr);

  // ── Blink ─────────────────────────────────────────────────────────────
  // Start blinking the given colour at halfPeriodMs on / halfPeriodMs off.
  void startBlink(uint32_t halfPeriodMs, bool r, bool g, bool b);

  // Change blink speed without resetting the on/off phase.
  // If the LED is not currently blinking, this call is a no-op.
  void setBlinkPeriod(uint32_t halfPeriodMs);

  bool isBlinking() const { return m_mode == Mode::Blink; }

private:
  enum class Mode { Idle, Flash, Blink };

  static void onTimer(void *ctx);
  void setRaw(bool r, bool g, bool b);
  void advanceFlashStep();

  GpioPin m_r, m_g, m_b;
  SoftwareTimer *m_timer = nullptr;
  Mode m_mode = Mode::Idle;

  // Flash state
  const FlashStep *m_flashSteps = nullptr;
  uint8_t m_flashCount = 0;
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
