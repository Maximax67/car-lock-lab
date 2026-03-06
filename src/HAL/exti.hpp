#pragma once

#include "gpio.hpp"
#include "stm32f103x6.h"

class ExtiPin {
public:
  enum class Trigger { Rising, Falling, Both };
  using Callback = void (*)(void *ctx) noexcept;

  ExtiPin() = default;

  void init(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
            GpioPull pull, Trigger trigger) noexcept;

  ExtiPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
          GpioPull pull, Trigger trigger) noexcept;

  void setCallback(Callback cb, void *ctx = nullptr) noexcept;

  void enable() noexcept;
  void disable() noexcept;

  [[nodiscard]] bool read() const noexcept { return m_gpio.read(); }

  // Called from the ISR dispatcher — clears the pending bit and fires the
  // registered callback.
  void handleIrq() noexcept;

  static void dispatch(uint8_t pin) noexcept;
  static void dispatchRange(uint8_t first, uint8_t last) noexcept;

  static ExtiPin *s_registry[16];

private:
  void configureAfio() noexcept;
  void configureLine() noexcept;
  void configureNvic() noexcept;

  [[nodiscard]] IRQn_Type irqn() const noexcept;
  [[nodiscard]] uint8_t portSource() const noexcept;

  GpioPin m_gpio;
  uint8_t m_pin = 0xFF; // 0xFF = uninitialised sentinel
  Trigger m_trigger = Trigger::Both;
  Callback m_callback = nullptr;
  void *m_ctx = nullptr;
};
