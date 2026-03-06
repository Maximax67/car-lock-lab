#include "rgb_led.hpp"
#include "HAL/hal_assert.hpp"

void RgbLed::init(GPIO_TypeDef *rPort, uint8_t rPin, GPIO_TypeDef *gPort,
                  uint8_t gPin, GPIO_TypeDef *bPort, uint8_t bPin,
                  SoftwareTimer *timer) noexcept {
  m_r.initAsOutput(rPort, rPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_g.initAsOutput(gPort, gPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_b.initAsOutput(bPort, bPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_timer = timer;
  setRaw(false, false, false);
}

void RgbLed::setColor(bool r, bool g, bool b) noexcept {
  m_timer->stop();
  m_mode = Mode::Idle;
  setRaw(r, g, b);
}

void RgbLed::off() noexcept { setColor(false, false, false); }

void RgbLed::playFlash(std::span<const FlashStep> steps, bool r, bool g, bool b,
                       bool repeat, Callback onDone, void *onDoneCtx) noexcept {
  HAL_ASSERT(!steps.empty());
  m_timer->stop();
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
  m_timer->start(steps[0].onMs, /*oneShot=*/true, onTimer, this);
}

void RgbLed::startBlink(uint32_t halfPeriodMs, bool r, bool g,
                        bool b) noexcept {
  m_timer->stop();
  m_blinkHalfPeriod = halfPeriodMs;
  m_blinkR = r;
  m_blinkG = g;
  m_blinkB = b;
  m_blinkState = true;
  m_mode = Mode::Blink;
  setRaw(r, g, b);
  m_timer->start(halfPeriodMs, /*oneShot=*/true, onTimer, this);
}

void RgbLed::setBlinkPeriod(uint32_t halfPeriodMs) noexcept {
  if (m_mode != Mode::Blink) {
    return;
  }

  m_blinkHalfPeriod = halfPeriodMs;
}

void RgbLed::setRaw(bool r, bool g, bool b) noexcept {
  r ? m_r.on() : m_r.off();
  g ? m_g.on() : m_g.off();
  b ? m_b.on() : m_b.off();
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
  m_timer->start(m_flashSteps[m_flashStep].onMs, /*oneShot=*/true, onTimer,
                 this);
}

void RgbLed::onTimer(void *ctx) noexcept {
  auto *self = static_cast<RgbLed *>(ctx);

  switch (self->m_mode) {
  case Mode::Blink:
    self->m_blinkState = !self->m_blinkState;
    if (self->m_blinkState) {
      self->setRaw(self->m_blinkR, self->m_blinkG, self->m_blinkB);
    } else {
      self->setRaw(false, false, false);
    }
    // Self-reschedule with the latest half-period (may have been updated since
    // the last tick by setBlinkPeriod().
    self->m_timer->start(self->m_blinkHalfPeriod, /*oneShot=*/true, onTimer,
                         self);
    break;

  case Mode::Flash:
    if (self->m_flashInOn) {
      // ON phase ended — turn off.
      self->setRaw(false, false, false);
      const uint32_t offMs = self->m_flashSteps[self->m_flashStep].offMs;
      if (offMs > 0u) {
        self->m_flashInOn = false;
        self->m_timer->start(offMs, /*oneShot=*/true, onTimer, self);
      } else {
        self->advanceFlashStep();
      }
    } else {
      // OFF phase ended — advance.
      self->advanceFlashStep();
    }
    break;

  case Mode::Idle:
    break;
  }
}
