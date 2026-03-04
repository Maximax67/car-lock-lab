#include "timer_manager.hpp"

TimerManager &TimerManager::instance() {
  static TimerManager inst;
  return inst;
}

void TimerManager::init(TIM_TypeDef *tim, uint32_t timClkHz,
                        uint8_t nvicPriority) {
  m_hwTimer.setCallback(onHwTick, this);
  m_hwTimer.init(tim, timClkHz, nvicPriority);
  m_hwTimer.start();
}

void TimerManager::handleTimerIrq() { m_hwTimer.handleIrq(); }

SoftwareTimer *TimerManager::allocate() {
  for (uint8_t i = 0; i < MAX_SW_TIMERS; ++i) {
    if (!m_used[i]) {
      m_used[i] = true;
      return &m_pool[i];
    }
  }
  // Pool exhausted — halt so we notice during development
  while (true) {
    __NOP();
  }
  return nullptr;
}

// Runs inside TIM2 ISR every 1 ms
void TimerManager::onHwTick(void *ctx) {
  auto *self = static_cast<TimerManager *>(ctx);
  self->m_ticks = self->m_ticks + 1;

  for (uint8_t i = 0; i < MAX_SW_TIMERS; ++i) {
    if (self->m_used[i]) {
      self->m_pool[i]._tick();
    }
  }
}
