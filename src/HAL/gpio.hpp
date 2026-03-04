#pragma once

#include "stm32f103x6.h"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// GpioPin — thin wrapper around a single STM32F1 GPIO pin.
//
// Two construction styles are supported:
//   1. One-shot (constructor does full init) — for standalone objects.
//   2. Two-phase (default ctor + initAsInput/initAsOutput) — for class members
//      that must be default-constructed before their port/pin are known.
// ─────────────────────────────────────────────────────────────────────────────

enum class GpioMode : uint8_t {
  Input = 0b00,
  Output10Mhz = 0b01,
  Output2Mhz = 0b10,
  Output50Mhz = 0b11,
};

enum class GpioPull : uint8_t {
  No,
  Up,
  Down,
};

enum class GpioInputMode : uint8_t {
  Analog = 0b00,
  Floating = 0b01,
  PullUpDown = 0b10,
};

enum class GpioOutputMode : uint8_t {
  PushPull = 0b00,
  OpenDrain = 0b01,
  AlternateFunctionPushPull = 0b10,
  AlternateFunctionOpenDrain = 0b11,
};

class GpioPin {
public:
  // ── Two-phase construction (default + explicit init) ─────────────────
  GpioPin() = default;

  void initAsInput(GPIO_TypeDef *port, uint8_t pin, GpioInputMode mode,
                   GpioPull pull = GpioPull::No);
  void initAsOutput(GPIO_TypeDef *port, uint8_t pin, GpioMode speed,
                    GpioOutputMode outMode);

  // ── One-shot construction (legacy / convenience) ──────────────────────
  GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
          GpioPull pull = GpioPull::No);
  GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioMode outputSpeed,
          GpioOutputMode outputMode);

  // ── Operations ────────────────────────────────────────────────────────
  void on();
  void off();
  void toggle();
  [[nodiscard]] bool read() const;

  GPIO_TypeDef *port() const { return m_port; }
  uint8_t pin() const { return m_pin; }

private:
  static void setValue(__IO uint32_t *reg, uint32_t bitsPerValue,
                       uint32_t value, uint8_t bitPosition);

  void enableClock();
  void configure(uint32_t mode, uint32_t cnf);

  GPIO_TypeDef *m_port = nullptr;
  uint8_t m_pin = 0;
};
