#include "software_timer.hpp"
#include "stm32f103x6.h"

void SoftwareTimer::start(uint32_t intervalMs, bool oneShot, Callback cb,
                          void *ctx) noexcept {
  // TIM2 (priority 0) can preempt an EXTI callback (priority 1) mid-write.
  // Disabling IRQs briefly makes the write atomic from TIM2's perspective.
  __disable_irq();
  m_interval = intervalMs;
  m_elapsed = 0;
  m_oneShot = oneShot;
  m_cb = cb;
  m_ctx = ctx;
  m_running = true;
  __enable_irq();
}

void SoftwareTimer::stop() noexcept {
  __disable_irq();
  m_running = false;
  m_elapsed = 0;
  __enable_irq();
}

void SoftwareTimer::restart() noexcept {
  __disable_irq();
  m_elapsed = 0;
  m_running = true;
  __enable_irq();
}

void SoftwareTimer::tick() noexcept {
  if (!m_running) {
    return;
  }

  auto elapsed = m_elapsed + 1;
  m_elapsed = elapsed;

  if (elapsed < m_interval) {
    return;
  }

  // Interval reached — commit the state change before firing the callback so
  // that the callback is free to call start() / stop() without racing.
  if (m_oneShot) {
    m_running = false;
  }

  m_elapsed = 0;

  if (m_cb) {
    m_cb(m_ctx);
  }
}
