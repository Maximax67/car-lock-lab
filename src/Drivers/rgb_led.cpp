#include "rgb_led.hpp"

void RgbLed::init(GPIO_TypeDef *rPort, uint8_t rPin, GPIO_TypeDef *gPort,
                  uint8_t gPin, GPIO_TypeDef *bPort, uint8_t bPin,
                  SoftwareTimer *timer) {
  m_r.initAsOutput(rPort, rPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_g.initAsOutput(gPort, gPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_b.initAsOutput(bPort, bPin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_timer = timer;
  setRaw(false, false, false);
}

// ── Solid colour ─────────────────────────────────────────────────────────────

void RgbLed::setColor(bool r, bool g, bool b) {
  m_timer->stop();
  m_mode = Mode::Idle;
  setRaw(r, g, b);
}

void RgbLed::off() { setColor(false, false, false); }

// ── Flash pattern
// ─────────────────────────────────────────────────────────────

void RgbLed::playFlash(const FlashStep *steps, uint8_t count, bool r, bool g,
                       bool b, bool repeat, Callback onDone, void *onDoneCtx) {
  m_timer->stop();
  m_flashSteps = steps;
  m_flashCount = count;
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

// ── Blink
// ─────────────────────────────────────────────────────────────────────
//
// Uses a self-rescheduling ONE-SHOT timer instead of a repeating timer.
//
// Why: setBlinkPeriod() is called every 100 ms during PreAlarm to smoothly
// accelerate the blink rate.  With a repeating timer the only way to change
// the period is to stop + restart, which resets the countdown to zero and
// means the timer never fires when the update interval (100 ms) is shorter
// than the blink half-period (up to 500 ms).
//
// With a one-shot that reschedules itself in onTimer(), setBlinkPeriod()
// only needs to store the new value — the next self-scheduled tick picks it
// up automatically.  The LED therefore always toggles correctly.

void RgbLed::startBlink(uint32_t halfPeriodMs, bool r, bool g, bool b) {
  m_timer->stop();
  m_blinkHalfPeriod = halfPeriodMs;
  m_blinkR = r;
  m_blinkG = g;
  m_blinkB = b;
  m_blinkState = true;
  m_mode = Mode::Blink;
  setRaw(r, g, b);
  // One-shot: onTimer will reschedule itself after each toggle.
  m_timer->start(halfPeriodMs, /*oneShot=*/true, onTimer, this);
}

void RgbLed::setBlinkPeriod(uint32_t halfPeriodMs) {
  if (m_mode != Mode::Blink)
    return;
  // Just store the new period.  The running one-shot will pick it up when
  // it fires and self-reschedules — no stop/restart needed.
  m_blinkHalfPeriod = halfPeriodMs;
}

// ── Private helpers
// ───────────────────────────────────────────────────────────

void RgbLed::setRaw(bool r, bool g, bool b) {
  r ? m_r.on() : m_r.off();
  g ? m_g.on() : m_g.off();
  b ? m_b.on() : m_b.off();
}

void RgbLed::advanceFlashStep() {
  ++m_flashStep;
  if (m_flashStep >= m_flashCount) {
    if (m_flashRepeat) {
      m_flashStep = 0;
    } else {
      // Pattern finished — go idle.
      m_mode = Mode::Idle;
      setRaw(false, false, false);

      // Fire completion callback after state is fully settled.
      if (m_flashOnDone)
        m_flashOnDone(m_flashOnDoneCtx);
      return;
    }
  }
  m_flashInOn = true;
  setRaw(m_flashR, m_flashG, m_flashB);
  m_timer->start(m_flashSteps[m_flashStep].onMs, /*oneShot=*/true, onTimer,
                 this);
}

// ── Timer callback (TIM2 ISR context) ────────────────────────────────────────

void RgbLed::onTimer(void *ctx) {
  auto *self = static_cast<RgbLed *>(ctx);

  switch (self->m_mode) {
  case Mode::Blink:
    // Toggle the LED.
    self->m_blinkState = !self->m_blinkState;
    if (self->m_blinkState)
      self->setRaw(self->m_blinkR, self->m_blinkG, self->m_blinkB);
    else
      self->setRaw(false, false, false);
    // Self-reschedule using the latest half-period (may have been updated by
    // setBlinkPeriod() since the last tick — that's the whole point).
    self->m_timer->start(self->m_blinkHalfPeriod, /*oneShot=*/true, onTimer,
                         self);
    break;

  case Mode::Flash:
    if (self->m_flashInOn) {
      // ON phase ended — turn off.
      self->setRaw(false, false, false);
      uint32_t offMs = self->m_flashSteps[self->m_flashStep].offMs;
      if (offMs > 0) {
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
