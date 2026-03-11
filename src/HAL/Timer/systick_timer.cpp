#include "systick_timer.hpp"

void SysTickTimer::init(uint32_t coreClkHz, uint8_t nvicPriority) noexcept {
  SysTick->LOAD = coreClkHz / 1000u - 1u;
  SysTick->VAL = 0u;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;

  NVIC_SetPriority(SysTick_IRQn, nvicPriority);
}

void SysTickTimer::setCallback(Callback cb, void *ctx) noexcept {
  m_cb = cb;
  m_ctx = ctx;
}

void SysTickTimer::handleIrq() noexcept {
  if (m_cb) {
    m_cb(m_ctx);
  }
}
