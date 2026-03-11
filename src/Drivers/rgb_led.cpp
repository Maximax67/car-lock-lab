#include "rgb_led.hpp"
#include "HAL/hal_assert.hpp"

RgbLed *RgbLed::s_instance = nullptr;

void RgbLed::init(GPIO_TypeDef *rPort, uint8_t rPin, GPIO_TypeDef *gPort,
                  uint8_t gPin, GPIO_TypeDef *bPort, uint8_t bPin,
                  TIM_TypeDef *blinkTim, uint32_t blinkTimClkHz,
                  uint8_t blinkNvicPri, SoftwareTimer *flashTimer) noexcept {
  m_r.initAsOutput(rPort, rPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_g.initAsOutput(gPort, gPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_b.initAsOutput(bPort, bPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);

  m_blinkTim = blinkTim;
  m_flashTimer = flashTimer;
  s_instance = this;

  initBlinkTim(blinkTimClkHz, blinkNvicPri);
  setRaw(false, false, false);
}

void RgbLed::initBlinkTim(uint32_t clkHz, uint8_t nvicPri) noexcept {
  if (m_blinkTim == TIM2) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  } else if (m_blinkTim == TIM3) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  }

  m_blinkTim->CR1 = 0;
  m_blinkTim->PSC = static_cast<uint16_t>(clkHz / 1000u - 1u);
  m_blinkTim->ARR = 499u;
  m_blinkTim->CNT = 0u;
  m_blinkTim->DIER = TIM_DIER_UIE;
  m_blinkTim->SR = 0u;

  const IRQn_Type irq = (m_blinkTim == TIM2)   ? TIM2_IRQn
                        : (m_blinkTim == TIM3) ? TIM3_IRQn
                                               : TIM2_IRQn;
  NVIC_SetPriority(irq, nvicPri);
  NVIC_EnableIRQ(irq);
}

void RgbLed::startBlinkTim(uint32_t halfPeriodMs) noexcept {
  m_blinkTim->ARR = static_cast<uint16_t>(halfPeriodMs - 1u);
  m_blinkTim->CNT = 0u;
  m_blinkTim->SR = 0u;
  m_blinkTim->CR1 |= TIM_CR1_CEN;
}

void RgbLed::stopBlinkTim() noexcept {
  m_blinkTim->CR1 &= ~TIM_CR1_CEN;
  m_blinkTim->SR = 0u;
}

void RgbLed::setColor(bool r, bool g, bool b) noexcept {
  stopBlinkTim();
  m_flashTimer->stop();
  m_mode = Mode::Idle;
  setRaw(r, g, b);
}

void RgbLed::off() noexcept { setColor(false, false, false); }

void RgbLed::playFlash(std::span<const FlashStep> steps, bool r, bool g, bool b,
                       bool repeat, Callback onDone, void *onDoneCtx) noexcept {
  HAL_ASSERT(!steps.empty());
  stopBlinkTim();
  m_flashTimer->stop();

  m_flashSteps = steps;
  m_flashRepeat = repeat;
  m_flashStep = 0;
  m_flashR = r;
  m_flashG = g;
  m_flashB = b;
  m_flashOnDone = onDone;
  m_flashOnDoneCtx = onDoneCtx;
  m_mode = Mode::Flash;
  m_flashInOn = true;

  setRaw(r, g, b);
  m_flashTimer->start(steps[0].onMs, /*oneShot=*/true, onFlashTimer, this);
}

void RgbLed::advanceFlashStep() noexcept {
  ++m_flashStep;
  if (m_flashStep >= m_flashSteps.size()) {
    if (m_flashRepeat) {
      m_flashStep = 0;
    } else {
      m_mode = Mode::Idle;
      setRaw(false, false, false);
      if (m_flashOnDone) {
        m_flashOnDone(m_flashOnDoneCtx);
      }
      return;
    }
  }
  m_flashInOn = true;
  setRaw(m_flashR, m_flashG, m_flashB);
  m_flashTimer->start(m_flashSteps[m_flashStep].onMs, /*oneShot=*/true,
                      onFlashTimer, this);
}

void RgbLed::onFlashTimer(void *ctx) noexcept {
  auto *self = static_cast<RgbLed *>(ctx);

  if (self->m_flashInOn) {
    // ON phase ended — turn off.
    self->setRaw(false, false, false);
    const uint32_t offMs = self->m_flashSteps[self->m_flashStep].offMs;
    if (offMs > 0u) {
      self->m_flashInOn = false;
      self->m_flashTimer->start(offMs, /*oneShot=*/true, onFlashTimer, self);
    } else {
      // Zero gap — jump straight to the next step.
      self->advanceFlashStep();
    }
  } else {
    // OFF phase ended — advance to the next step.
    self->advanceFlashStep();
  }
}

void RgbLed::startBlink(uint32_t halfPeriodMs, bool r, bool g,
                        bool b) noexcept {
  stopBlinkTim();
  m_flashTimer->stop();

  m_blinkHalfPeriod = halfPeriodMs;
  m_blinkR = r;
  m_blinkG = g;
  m_blinkB = b;
  m_blinkState = true;
  m_mode = Mode::Blink;

  setRaw(r, g, b);
  startBlinkTim(halfPeriodMs);
}

void RgbLed::setBlinkPeriod(uint32_t halfPeriodMs) noexcept {
  if (m_mode != Mode::Blink) {
    return;
  }
  m_blinkHalfPeriod = halfPeriodMs;
  m_blinkTim->ARR = static_cast<uint16_t>(halfPeriodMs - 1u);
}

void RgbLed::dispatchBlinkIrq() noexcept {
  if (s_instance) {
    s_instance->handleBlinkIrq();
  }
}

void RgbLed::handleBlinkIrq() noexcept {
  if (!(m_blinkTim->SR & TIM_SR_UIF)) {
    return;
  }
  m_blinkTim->SR &= ~TIM_SR_UIF;

  if (m_mode != Mode::Blink) {
    return;
  }

  m_blinkState = !m_blinkState;
  if (m_blinkState) {
    setRaw(m_blinkR, m_blinkG, m_blinkB);
  } else {
    setRaw(false, false, false);
  }
}

void RgbLed::setRaw(bool r, bool g, bool b) noexcept {
  r ? m_r.on() : m_r.off();
  g ? m_g.on() : m_g.off();
  b ? m_b.on() : m_b.off();
}
