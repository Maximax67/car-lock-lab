#include "buzzer.hpp"
#include "HAL/hal_assert.hpp"

void Buzzer::init(GPIO_TypeDef *port, uint8_t pin,
                  SoftwareTimer *timer) noexcept {
  m_pin.initAsOutput(port, pin, GpioMode::Output2Mhz, GpioOutputMode::PushPull);
  m_pin.off();
  m_timer = timer;
}

// ── Public API ───────────────────────────────────────────────────────────────

void Buzzer::playPattern(std::span<const BeepPattern> pattern,
                         bool repeat) noexcept {
  HAL_ASSERT(!pattern.empty());
  stop();
  m_pattern = pattern;
  m_repeat = repeat;
  m_step = 0;
  m_running = true;

  // Kick off the first ON phase immediately.
  m_inOn = true;
  m_pin.on();
  m_timer->start(m_pattern[0].onMs, /*oneShot=*/true, onTimer, this);
}

void Buzzer::beep(uint32_t durationMs) noexcept {
  m_singleBeep = {durationMs, 0};
  playPattern(std::span<const BeepPattern>{&m_singleBeep, 1u}, false);
}

void Buzzer::stop() noexcept {
  m_timer->stop();
  m_pin.off();
  m_running = false;
}

// ── Internal step advancement ────────────────────────────────────────────────

void Buzzer::advanceStep() noexcept {
  ++m_step;
  if (m_step >= m_pattern.size()) {
    if (m_repeat) {
      m_step = 0;
    } else {
      m_running = false; // pattern finished — buzzer already off
      return;
    }
  }
  // Start the next ON phase.
  m_inOn = true;
  m_pin.on();
  m_timer->start(m_pattern[m_step].onMs, /*oneShot=*/true, onTimer, this);
}

// ── Timer callback (TIM2 ISR context) ───────────────────────────────────────

void Buzzer::onTimer(void *ctx) noexcept {
  auto *self = static_cast<Buzzer *>(ctx);

  if (self->m_inOn) {
    // ON phase ended — turn off.
    self->m_pin.off();
    const uint32_t offMs = self->m_pattern[self->m_step].offMs;
    if (offMs > 0u) {
      // Schedule the OFF phase.
      self->m_inOn = false;
      self->m_timer->start(offMs, /*oneShot=*/true, onTimer, self);
    } else {
      // No gap — advance directly to the next step.
      self->advanceStep();
    }
  } else {
    // OFF phase ended — advance to the next step.
    self->advanceStep();
  }
}
