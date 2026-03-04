#pragma once

#include "gpio.hpp"
#include "stm32f103x6.h"

// ─────────────────────────────────────────────────────────────────────────────
// ExtiPin — attaches an EXTI interrupt to any GPIO pin.
//
// Two construction styles:
//   One-shot:   ExtiPin btn(GPIOA, 0, GpioInputMode::PullUpDown, GpioPull::Up,
//                           ExtiPin::Trigger::Falling);
//   Two-phase:  ExtiPin btn;        // default ctor, no hardware touch
//               btn.init(GPIOA, 0, GpioInputMode::PullUpDown,
//                        GpioPull::Up, ExtiPin::Trigger::Falling);
//
// The static s_registry[16] maps EXTI line → ExtiPin* so ISR handlers can
// dispatch correctly.  ISR handlers must call dispatch() / dispatchRange().
// ─────────────────────────────────────────────────────────────────────────────
class ExtiPin {
public:
  enum class Trigger { Rising, Falling, Both };
  using Callback = void (*)(void *ctx);

  // Default ctor — no hardware initialisation, no registry entry.
  ExtiPin() = default;

  // Two-phase init (usable when ExtiPin is a class member).
  void init(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
            GpioPull pull, Trigger trigger);

  // One-shot construction (convenience — calls init() internally).
  ExtiPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
          GpioPull pull, Trigger trigger);

  void setCallback(Callback cb, void *ctx = nullptr);

  void enable();
  void disable();

  bool read() const { return m_gpio.read(); }

  // Called from ISR dispatcher — clears pending bit, fires callback.
  void handleIrq();

  static void dispatch(uint8_t pin);
  static void dispatchRange(uint8_t first, uint8_t last);

  static ExtiPin *s_registry[16];

private:
  void configureAfio();
  void configureLine();
  void configureNvic();
  IRQn_Type irqn() const;
  uint8_t portSource() const;

  GpioPin m_gpio;
  uint8_t m_pin = 0xFF; // 0xFF = uninitialised sentinel
  Trigger m_trigger = Trigger::Both;
  Callback m_callback = nullptr;
  void *m_ctx = nullptr;
};
