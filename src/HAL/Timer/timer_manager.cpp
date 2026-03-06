#include "timer_manager.hpp"
#include "HAL/hal_assert.hpp"

// Pool-size sanity check — if the timer budget changes, update the comment in
// the header to match and raise MAX_SW_TIMERS if needed.
static_assert(MAX_SW_TIMERS >= 15, "Timer pool too small");

TimerManager &TimerManager::instance() noexcept {
  static TimerManager inst;
  return inst;
}

void TimerManager::init(TIM_TypeDef *tim, uint32_t timClkHz,
                        uint8_t nvicPriority) noexcept {
  m_hwTimer.setCallback(onHwTick, this);
  m_hwTimer.init(tim, timClkHz, nvicPriority);
  m_hwTimer.start();
}

void TimerManager::handleTimerIrq() noexcept { m_hwTimer.handleIrq(); }

SoftwareTimer *TimerManager::allocate() noexcept {
  for (uint8_t i = 0; i < MAX_SW_TIMERS; ++i) {
    if (!m_used[i]) {
      m_used[i] = true;
      return &m_pool[i];
    }
  }
  halPanic(); // pool exhausted — raise MAX_SW_TIMERS or reduce driver count
}

void TimerManager::onHwTick(void *ctx) noexcept {
  auto *self = static_cast<TimerManager *>(ctx);
  self->m_ticks = self->m_ticks + 1;
  for (uint8_t i = 0; i < MAX_SW_TIMERS; ++i) {
    if (self->m_used[i]) {
      self->m_pool[i].tick();
    }
  }
}
