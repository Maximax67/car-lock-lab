#include "gpio.hpp"

// ── Two-phase init ──────────────────────────────────────────────────────────

void GpioPin::initAsInput(GPIO_TypeDef *port, uint8_t pin,
                          GpioInputMode inputMode, GpioPull pull) noexcept {
  m_port = port;
  m_pin = pin;
  enableClock();
  configure(static_cast<uint32_t>(GpioMode::Input),
            static_cast<uint32_t>(inputMode));

  if (inputMode == GpioInputMode::PullUpDown) {
    const uint32_t mask = 1UL << m_pin;
    if (pull == GpioPull::Up)
      m_port->BSRR = mask;
    else if (pull == GpioPull::Down)
      m_port->BSRR = mask << 16;
  }
}

void GpioPin::initAsOutput(GPIO_TypeDef *port, uint8_t pin, GpioMode speed,
                           GpioOutputMode outMode) noexcept {
  m_port = port;
  m_pin = pin;
  enableClock();
  configure(static_cast<uint32_t>(speed), static_cast<uint32_t>(outMode));
}

// ── One-shot constructors ───────────────────────────────────────────────────

GpioPin::GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
                 GpioPull pull) noexcept {
  initAsInput(port, pin, inputMode, pull);
}

GpioPin::GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioMode outputSpeed,
                 GpioOutputMode outputMode) noexcept {
  initAsOutput(port, pin, outputSpeed, outputMode);
}

// ── Private helpers ─────────────────────────────────────────────────────────

void GpioPin::enableClock() noexcept {
  const uint32_t portIndex = (reinterpret_cast<uint32_t>(m_port) - GPIOA_BASE) /
                             (GPIOB_BASE - GPIOA_BASE);
  RCC->APB2ENR |= (0x1UL << (RCC_APB2ENR_IOPAEN_Pos + portIndex));
}

void GpioPin::configure(uint32_t mode, uint32_t cnf) noexcept {
  __IO uint32_t *reg = (m_pin >= 8) ? &m_port->CRH : &m_port->CRL;
  const uint8_t pinIndex = m_pin % 8u;
  setValue(reg, 2u, mode, pinIndex * 4u);
  setValue(reg, 2u, cnf, pinIndex * 4u + 2u);
}

// ── Output operations ───────────────────────────────────────────────────────

void GpioPin::on() noexcept { m_port->BSRR = 1UL << m_pin; }
void GpioPin::off() noexcept { m_port->BSRR = 1UL << (m_pin + 16u); }

void GpioPin::toggle() noexcept {
  const uint32_t mask = 1UL << m_pin;
  m_port->BSRR = (m_port->ODR & mask) ? (mask << 16) : mask;
}

[[nodiscard]] bool GpioPin::read() const noexcept {
  return (m_port->IDR & (1UL << m_pin)) != 0u;
}

void GpioPin::setValue(__IO uint32_t *reg, uint32_t bitsPerValue,
                       uint32_t value, uint8_t bitPosition) noexcept {
  const uint32_t mask = (1UL << bitsPerValue) - 1UL;
  *reg = (*reg & ~(mask << bitPosition)) | ((value & mask) << bitPosition);
}
