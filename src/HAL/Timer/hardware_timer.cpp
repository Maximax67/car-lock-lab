#include "hardware_timer.hpp"

void HardwareTimer::init(TIM_TypeDef *tim, uint32_t timClkHz,
                         uint8_t nvicPriority) {
  m_tim = tim;
  enableClock();

  m_tim->CR1 = 0; // stop and clear control register

  m_tim->PSC = static_cast<uint16_t>(timClkHz / 10'000u - 1u);
  m_tim->ARR = 9u;
  m_tim->CNT = 0;

  m_tim->DIER = TIM_DIER_UIE; // enable update interrupt
  m_tim->SR = 0;              // clear any stale flags

  // Configure NVIC
  IRQn_Type irq = irqnFor(tim);
  NVIC_SetPriority(irq, nvicPriority);
  NVIC_EnableIRQ(irq);
}

void HardwareTimer::start() {
  if (m_tim)
    m_tim->CR1 |= TIM_CR1_CEN;
}

void HardwareTimer::stop() {
  if (m_tim)
    m_tim->CR1 &= ~TIM_CR1_CEN;
}

void HardwareTimer::setCallback(Callback cb, void *ctx) {
  m_cb = cb;
  m_ctx = ctx;
}

void HardwareTimer::handleIrq() {
  if (!(m_tim->SR & TIM_SR_UIF))
    return;

  m_tim->SR &= ~TIM_SR_UIF;

  if (m_cb)
    m_cb(m_ctx);
}

void HardwareTimer::enableClock() {
  if (m_tim == TIM1)
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  else if (m_tim == TIM2)
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  else if (m_tim == TIM3)
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
}

IRQn_Type HardwareTimer::irqnFor(TIM_TypeDef *tim) const {
  if (tim == TIM1)
    return TIM1_UP_IRQn;
  if (tim == TIM2)
    return TIM2_IRQn;
  if (tim == TIM3)
    return TIM3_IRQn;

#ifdef TIM4
  if (tim == TIM4)
    return TIM4_IRQn;
#endif
#ifdef TIM5
  if (tim == TIM5)
    return TIM5_IRQn;
#endif
#ifdef TIM6
  if (tim == TIM6)
    return TIM6_IRQn;
#endif
#ifdef TIM7
  if (tim == TIM7)
    return TIM7_IRQn;
#endif

  __BKPT(0); // break into debugger
  while (true)
    ; // safety halt
}
