#include "software_timer.hpp"
#include "stm32f103x6.h"

void SoftwareTimer::start(uint32_t intervalMs, bool oneShot, Callback cb,
                          void *ctx) {
  // Guard: TIM2 IRQ (priority 0) can preempt EXTI (priority 1).
  // If an EXTI callback calls start() while TIM2 fires mid-write,
  // the tick would see a half-written timer state. Disable IRQs briefly.
  __disable_irq();
  m_interval = intervalMs;
  m_elapsed = 0;
  m_oneShot = oneShot;
  m_cb = cb;
  m_ctx = ctx;
  m_running = true;
  __enable_irq();
}

void SoftwareTimer::stop() {
  __disable_irq();
  m_running = false;
  m_elapsed = 0;
  __enable_irq();
}

void SoftwareTimer::restart() {
  __disable_irq();
  m_elapsed = 0;
  m_running = true;
  __enable_irq();
}

void SoftwareTimer::_tick() {
  // Called from TIM2 ISR — keep this fast, no blocking
  if (!m_running)
    return;

  ++m_elapsed;

  if (m_elapsed < m_interval)
    return;

  // Interval reached — fire callback
  if (m_oneShot) {
    m_running = false;
    m_elapsed = 0;
  } else {
    m_elapsed = 0; // auto-reload for repeating mode
  }

  // Callback may call start() / stop() on this or other timers — that's fine
  // because we've already committed our state change above.
  if (m_cb) {
    m_cb(m_ctx);
  }
}
