#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include <cstdint>
#include <span>

struct BeepPattern {
  uint32_t onMs;
  uint32_t offMs;
};

class Buzzer {
public:
  void init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer) noexcept;

  void playPattern(std::span<const BeepPattern> pattern,
                   bool repeat = false) noexcept;
  void beep(uint32_t durationMs) noexcept;
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

  BeepPattern m_singleBeep{};
};
