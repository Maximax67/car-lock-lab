#pragma once

#include "HAL/Timer/software_timer.hpp"
#include "HAL/gpio.hpp"
#include "stm32f103x6.h"
#include <cstdint>
#include <span>

struct FlashStep {
  uint32_t onMs;
  uint32_t offMs;
};

class RgbLed {
public:
  using Callback = void (*)(void *ctx) noexcept;

  void init(GPIO_TypeDef *rPort, uint8_t rPin, GPIO_TypeDef *gPort,
            uint8_t gPin, GPIO_TypeDef *bPort, uint8_t bPin,
            TIM_TypeDef *blinkTim, uint32_t blinkTimClkHz, uint8_t blinkNvicPri,
            SoftwareTimer *flashTimer) noexcept;

  void setColor(bool r, bool g, bool b) noexcept;
  void off() noexcept;

  void playFlash(std::span<const FlashStep> steps, bool r, bool g, bool b,
                 bool repeat = false, Callback onDone = nullptr,
                 void *onDoneCtx = nullptr) noexcept;

  void startBlink(uint32_t halfPeriodMs, bool r, bool g, bool b) noexcept;

  void setBlinkPeriod(uint32_t halfPeriodMs) noexcept;

  [[nodiscard]] bool isBlinking() const noexcept {
    return m_mode == Mode::Blink;
  }

  static void dispatchBlinkIrq() noexcept;

private:
  enum class Mode { Idle, Flash, Blink };

  void initBlinkTim(uint32_t clkHz, uint8_t nvicPri) noexcept;
  void startBlinkTim(uint32_t halfPeriodMs) noexcept;
  void stopBlinkTim() noexcept;

  void handleBlinkIrq() noexcept;

  static void onFlashTimer(void *ctx) noexcept;
  void advanceFlashStep() noexcept;

  void setRaw(bool r, bool g, bool b) noexcept;

  static RgbLed *s_instance;

  GpioPin m_r, m_g, m_b;
  TIM_TypeDef *m_blinkTim = nullptr;
  SoftwareTimer *m_flashTimer = nullptr;
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
