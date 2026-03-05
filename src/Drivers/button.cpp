#include "button.hpp"

// ── Init ─────────────────────────────────────────────────────────────────────

void Button::init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
                  SoftwareTimer *windowTimer, uint32_t debounceMs,
                  uint32_t doubleClickWindowMs) {
  m_debounceTimer = debounceTimer;
  m_windowTimer = windowTimer;
  m_debounceMs = debounceMs;
  m_windowMs = doubleClickWindowMs;

  // Pull-up: pin is HIGH at rest, LOW when pressed.
  // Falling edge = press event — that is all we need.
  m_exti.init(port, pin, GpioInputMode::PullUpDown, GpioPull::Up,
              ExtiPin::Trigger::Falling);
  m_exti.setCallback(onExtiIrq, this);
}

// ── Callback setters
// ──────────────────────────────────────────────────────────

void Button::setOnSingleClick(Callback cb, void *ctx) {
  m_singleClickCb = cb;
  m_singleClickCtx = ctx;
}

void Button::setOnDoubleClick(Callback cb, void *ctx) {
  m_doubleClickCb = cb;
  m_doubleClickCtx = ctx;
}

// ── EXTI fires (falling edge = press) ────────────────────────────────────────
//   Mask the EXTI line immediately so bounces are ignored, then start the
//   debounce timer.  The line is re-enabled only after the debounce expires.

void Button::onExtiIrq(void *ctx) {
  auto *self = static_cast<Button *>(ctx);

  // Mask EXTI to suppress any bounce during the debounce window.
  self->m_exti.disable();

  // Start (or restart) the debounce one-shot.
  self->m_debounceTimer->start(self->m_debounceMs, /*oneShot=*/true, onDebounce,
                               self);
}

// ── Debounce expired
// ──────────────────────────────────────────────────────────
//   Re-enable the EXTI, then read the pin.  If it is still LOW the press is
//   genuine.  Count it and manage the double-click window.

void Button::onDebounce(void *ctx) {
  auto *self = static_cast<Button *>(ctx);

  // Re-arm the EXTI *before* deciding — so the next press is never missed.
  self->m_exti.enable();

  // Pin is LOW (active) ↔ button is still held → genuine click.
  if (!self->m_exti.read()) {
    self->m_pendingClicks = self->m_pendingClicks + 1;

    if (self->m_pendingClicks == 1) {
      if (self->m_doubleClickCb == nullptr) {
        // No double-click handler registered — fire single-click immediately,
        // no need to wait out the double-click window.
        self->m_pendingClicks = 0;
        if (self->m_singleClickCb)
          self->m_singleClickCb(self->m_singleClickCtx);
      } else {
        // Double-click handler exists — open the window and wait.
        self->m_windowTimer->start(self->m_windowMs, /*oneShot=*/true, onWindow,
                                   self);
      }
    } else {
      // Second (or more) click inside the window — double-click!
      self->m_windowTimer->stop();
      self->m_pendingClicks = 0;
      if (self->m_doubleClickCb)
        self->m_doubleClickCb(self->m_doubleClickCtx);
    }
  }
  // If pin is HIGH here the press was too short (bounce) — discard.
}

// ── Double-click window expired
// ───────────────────────────────────────────────
//   Only one confirmed click arrived → single-click event.

void Button::onWindow(void *ctx) {
  auto *self = static_cast<Button *>(ctx);
  if (self->m_pendingClicks == 1) {
    if (self->m_singleClickCb)
      self->m_singleClickCb(self->m_singleClickCtx);
  }
  self->m_pendingClicks = 0;
}
