#pragma once

#include "stm32f103x6.h"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// GpioPin — thin wrapper around a single STM32F1 GPIO pin.
//
// Two construction styles are supported:
//   1. Two-phase (default ctor + initAsInput / initAsOutput) — for class
//      members that must be default-constructed before port/pin are known.
//   2. One-shot (constructor does full init) — for standalone objects.
//
// All methods are noexcept: the class never allocates or throws.
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
  GpioPin() = default;

  void initAsInput(GPIO_TypeDef *port, uint8_t pin, GpioInputMode mode,
                   GpioPull pull = GpioPull::No) noexcept;
  void initAsOutput(GPIO_TypeDef *port, uint8_t pin, GpioMode speed,
                    GpioOutputMode outMode) noexcept;

  GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
          GpioPull pull = GpioPull::No) noexcept;
  GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioMode outputSpeed,
          GpioOutputMode outputMode) noexcept;

  void on() noexcept;
  void off() noexcept;
  void toggle() noexcept;

  [[nodiscard]] bool read() const noexcept;
  [[nodiscard]] GPIO_TypeDef *port() const noexcept { return m_port; }
  [[nodiscard]] uint8_t pin() const noexcept { return m_pin; }

private:
  static void setValue(__IO uint32_t *reg, uint32_t bitsPerValue,
                       uint32_t value, uint8_t bitPosition) noexcept;
  void enableClock() noexcept;
  void configure(uint32_t mode, uint32_t cnf) noexcept;

  GPIO_TypeDef *m_port = nullptr;
  uint8_t m_pin = 0;
};
