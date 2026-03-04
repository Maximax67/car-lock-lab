#include "buzzer.hpp"

void Buzzer::init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *timer) {
  m_pin.initAsOutput(port, pin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_pin.off();
  m_timer = timer;
}

// ── Public API ──────────────────────────────────────────────────────────────

void Buzzer::playPattern(const BeepPattern *pattern, uint8_t count,
                         bool repeat) {
  stop();
  m_pattern = pattern;
  m_count = count;
  m_repeat = repeat;
  m_step = 0;
  m_running = true;

  // Kick off the first ON phase.
  m_inOn = true;
  m_pin.on();
  m_timer->start(m_pattern[0].onMs, /*oneShot=*/true, onTimer, this);
}

void Buzzer::beep(uint32_t durationMs) {
  m_singleBeep = {durationMs, 0};
  playPattern(&m_singleBeep, 1, false);
}

void Buzzer::stop() {
  m_timer->stop();
  m_pin.off();
  m_running = false;
}

// ── Internal step advancement ───────────────────────────────────────────────

void Buzzer::advanceStep() {
  ++m_step;
  if (m_step >= m_count) {
    if (m_repeat) {
      m_step = 0;
    } else {
      m_running = false;
      return; // pattern finished — buzzer already off
    }
  }
  // Start next ON phase.
  m_inOn = true;
  m_pin.on();
  m_timer->start(m_pattern[m_step].onMs, /*oneShot=*/true, onTimer, this);
}

// ── Timer callback (runs in TIM2 ISR context) ───────────────────────────────

void Buzzer::onTimer(void *ctx) {
  auto *self = static_cast<Buzzer *>(ctx);

  if (self->m_inOn) {
    // ON phase ended — turn off.
    self->m_pin.off();
    uint32_t offMs = self->m_pattern[self->m_step].offMs;
    if (offMs > 0) {
      // Schedule OFF phase.
      self->m_inOn = false;
      self->m_timer->start(offMs, /*oneShot=*/true, onTimer, self);
    } else {
      // No gap — go directly to the next step.
      self->advanceStep();
    }
  } else {
    // OFF phase ended — advance to next step.
    self->advanceStep();
  }
}
