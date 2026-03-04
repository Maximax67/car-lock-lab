#include "gpio.hpp"

// ── Two-phase init ──────────────────────────────────────────────────────────

void GpioPin::initAsInput(GPIO_TypeDef *port, uint8_t pin,
                          GpioInputMode inputMode, GpioPull pull) {
  m_port = port;
  m_pin = pin;
  enableClock();
  configure(static_cast<uint32_t>(GpioMode::Input),
            static_cast<uint32_t>(inputMode));

  if (inputMode == GpioInputMode::PullUpDown) {
    uint32_t mask = 1UL << m_pin;
    if (pull == GpioPull::Up)
      m_port->BSRR = mask;
    else if (pull == GpioPull::Down)
      m_port->BSRR = mask << 16;
  }
}

void GpioPin::initAsOutput(GPIO_TypeDef *port, uint8_t pin, GpioMode speed,
                           GpioOutputMode outMode) {
  m_port = port;
  m_pin = pin;
  enableClock();
  configure(static_cast<uint32_t>(speed), static_cast<uint32_t>(outMode));
}

// ── One-shot constructors ───────────────────────────────────────────────────

GpioPin::GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioInputMode inputMode,
                 GpioPull pull) {
  initAsInput(port, pin, inputMode, pull);
}

GpioPin::GpioPin(GPIO_TypeDef *port, uint8_t pin, GpioMode outputSpeed,
                 GpioOutputMode outputMode) {
  initAsOutput(port, pin, outputSpeed, outputMode);
}

// ── Private helpers ─────────────────────────────────────────────────────────

void GpioPin::enableClock() {
  uint32_t portIndex = (reinterpret_cast<uint32_t>(m_port) - GPIOA_BASE) /
                       (GPIOB_BASE - GPIOA_BASE);
  RCC->APB2ENR |= (0x1UL << (RCC_APB2ENR_IOPAEN_Pos + portIndex));
}

void GpioPin::configure(uint32_t mode, uint32_t cnf) {
  __IO uint32_t *reg = (m_pin >= 8) ? &m_port->CRH : &m_port->CRL;
  uint8_t pinIndex = m_pin % 8;
  setValue(reg, 2, mode, pinIndex * 4);
  setValue(reg, 2, cnf, pinIndex * 4 + 2);
}

// ── Output operations ───────────────────────────────────────────────────────

void GpioPin::on() { m_port->BSRR = 1UL << m_pin; }
void GpioPin::off() { m_port->BSRR = 1UL << (m_pin + 16); }

void GpioPin::toggle() {
  uint32_t mask = 1UL << m_pin;
  if (m_port->ODR & mask)
    m_port->BSRR = mask << 16;
  else
    m_port->BSRR = mask;
}

[[nodiscard]] bool GpioPin::read() const {
  return (m_port->IDR & (1UL << m_pin)) != 0;
}

void GpioPin::setValue(__IO uint32_t *reg, uint32_t bitsPerValue,
                       uint32_t value, uint8_t bitPosition) {
  const uint32_t mask = (1UL << bitsPerValue) - 1UL;
  *reg = (*reg & ~(mask << bitPosition)) | ((value & mask) << bitPosition);
}
