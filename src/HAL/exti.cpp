#include "exti.hpp"

ExtiPin *ExtiPin::s_registry[16] = {};

// ── Two-phase init ──────────────────────────────────────────────────────────

void ExtiPin::init(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
                   GpioPull pull, Trigger trigger) noexcept {
  m_pin = pin;
  m_trigger = trigger;
  m_gpio.initAsInput(port, pin, inputMode, pull);
  s_registry[pin] = this;
  configureAfio();
  configureLine();
  configureNvic();
}

// ── One-shot constructor ─────────────────────────────────────────────────────

ExtiPin::ExtiPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
                 GpioPull pull, Trigger trigger) noexcept {
  init(port, pin, inputMode, pull, trigger);
}

// ── Public interface ────────────────────────────────────────────────────────

void ExtiPin::setCallback(Callback cb, void *ctx) noexcept {
  __disable_irq();
  m_callback = cb;
  m_ctx = ctx;
  __enable_irq();
}

void ExtiPin::enable() noexcept { EXTI->IMR |= (1UL << m_pin); }
void ExtiPin::disable() noexcept { EXTI->IMR &= ~(1UL << m_pin); }

void ExtiPin::handleIrq() noexcept {
  const uint32_t pending = EXTI->PR;
  if (pending & (1UL << m_pin)) {
    EXTI->PR = (1UL << m_pin); // clear by writing 1
    if (m_callback)
      m_callback(m_ctx);
  }
}

// ── ISR dispatch helpers ────────────────────────────────────────────────────

void ExtiPin::dispatch(uint8_t pin) noexcept {
  if (s_registry[pin])
    s_registry[pin]->handleIrq();
}

void ExtiPin::dispatchRange(uint8_t first, uint8_t last) noexcept {
  for (uint8_t i = first; i <= last; ++i)
    if ((EXTI->PR & (1UL << i)) && s_registry[i])
      s_registry[i]->handleIrq();
}

// ── Private helpers ─────────────────────────────────────────────────────────

uint8_t ExtiPin::portSource() const noexcept {
  // GPIO ports are spaced 0x400 bytes apart starting from GPIOA_BASE.
  constexpr uint32_t GPIO_PORT_STRIDE = GPIOB_BASE - GPIOA_BASE;
  return static_cast<uint8_t>(
      (reinterpret_cast<uint32_t>(m_gpio.port()) - GPIOA_BASE) /
      GPIO_PORT_STRIDE);
}

void ExtiPin::configureAfio() noexcept {
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
  const uint8_t crIdx = m_pin / 4u;
  const uint8_t bitPos = (m_pin % 4u) * 4u;
  const uint32_t mask = 0xFu << bitPos;
  AFIO->EXTICR[crIdx] = (AFIO->EXTICR[crIdx] & ~mask) |
                        (static_cast<uint32_t>(portSource()) << bitPos);
}

void ExtiPin::configureLine() noexcept {
  const uint32_t line = 1UL << m_pin;
  if (m_trigger == Trigger::Rising || m_trigger == Trigger::Both)
    EXTI->RTSR |= line;
  else
    EXTI->RTSR &= ~line;
  if (m_trigger == Trigger::Falling || m_trigger == Trigger::Both)
    EXTI->FTSR |= line;
  else
    EXTI->FTSR &= ~line;
  EXTI->PR = line;   // clear any stale pending bit
  EXTI->IMR |= line; // unmask — enable interrupt delivery
}

void ExtiPin::configureNvic() noexcept {
  const IRQn_Type irq = irqn();
  NVIC_SetPriority(irq, 1); // lower than TIM2 (0) so TIM2 can preempt EXTI
  NVIC_EnableIRQ(irq);
}

IRQn_Type ExtiPin::irqn() const noexcept {
  switch (m_pin) {
  case 0:
    return EXTI0_IRQn;
  case 1:
    return EXTI1_IRQn;
  case 2:
    return EXTI2_IRQn;
  case 3:
    return EXTI3_IRQn;
  case 4:
    return EXTI4_IRQn;
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    return EXTI9_5_IRQn;
  default:
    return EXTI15_10_IRQn;
  }
}
