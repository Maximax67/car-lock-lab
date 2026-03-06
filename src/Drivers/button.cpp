#include "button.hpp"

void Button::init(GPIO_TypeDef *port, uint8_t pin, SoftwareTimer *debounceTimer,
                  SoftwareTimer *windowTimer, uint32_t debounceMs,
                  uint32_t doubleClickWindowMs) noexcept {
  m_debounceTimer = debounceTimer;
  m_windowTimer = windowTimer;
  m_debounceMs = debounceMs;
  m_windowMs = doubleClickWindowMs;

  // Pull-up: pin HIGH at rest, LOW when pressed.
  // Falling edge = press — the only edge we need.
  m_exti.init(port, pin, GpioInputMode::PullUpDown, GpioPull::Up,
              ExtiPin::Trigger::Falling);
  m_exti.setCallback(onExtiIrq, this);
}

void Button::setOnSingleClick(Callback cb, void *ctx) noexcept {
  m_singleClickCb = cb;
  m_singleClickCtx = ctx;
}

void Button::setOnDoubleClick(Callback cb, void *ctx) noexcept {
  m_doubleClickCb = cb;
  m_doubleClickCtx = ctx;
}

void Button::onExtiIrq(void *ctx) noexcept {
  auto *self = static_cast<Button *>(ctx);
  self->m_exti.disable();
  self->m_debounceTimer->start(self->m_debounceMs, /*oneShot=*/true, onDebounce,
                               self);
}

void Button::onDebounce(void *ctx) noexcept {
  auto *self = static_cast<Button *>(ctx);
  self->m_exti.enable();

  if (!self->m_exti.read()) { // pin LOW = genuinely pressed
    self->m_pendingClicks = self->m_pendingClicks + 1;

    if (self->m_pendingClicks == 1u) {
      if (self->m_doubleClickCb == nullptr) {
        // No double-click handler registered — fire single-click immediately,
        // no need to wait out the window.
        self->m_pendingClicks = 0;
        if (self->m_singleClickCb) {
          self->m_singleClickCb(self->m_singleClickCtx);
        }
      } else {
        // Double-click handler exists — open the classification window.
        self->m_windowTimer->start(self->m_windowMs, /*oneShot=*/true, onWindow,
                                   self);
      }
    } else {
      // Second (or later) click inside the window — double-click confirmed.
      self->m_windowTimer->stop();
      self->m_pendingClicks = 0;
      if (self->m_doubleClickCb) {
        self->m_doubleClickCb(self->m_doubleClickCtx);
      }
    }
  }
  // If pin is HIGH here the press was too short (bounce) — discard.
}

void Button::onWindow(void *ctx) noexcept {
  auto *self = static_cast<Button *>(ctx);
  if (self->m_pendingClicks == 1u && self->m_singleClickCb) {
    self->m_singleClickCb(self->m_singleClickCtx);
  }
  self->m_pendingClicks = 0;
}
