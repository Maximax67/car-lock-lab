#include "App/car_alarm.hpp"
#include "Drivers/button.hpp"
#include "Drivers/buzzer.hpp"
#include "Drivers/motion_sensor.hpp"
#include "Drivers/relay.hpp"
#include "Drivers/rgb_led.hpp"
#include "HAL/Timer/timer_manager.hpp"
#include "App/config.hpp"
#include "stm32f103x6.h"

// ─────────────────────────────────────────────────────────────────────────────
// All top-level objects live in BSS (zero-initialised at startup).
// No heap is used anywhere in this project.
// ─────────────────────────────────────────────────────────────────────────────

// ── Drivers ──────────────────────────────────────────────────────────────────
static Button g_btn1;
static Button g_btn2;
static Button g_btn3;
static MotionSensor g_motion;
static Relay g_lockRelay;
static Relay g_cargoRelay;
static Buzzer g_buzzer;
static RgbLed g_led;

// ── Application ──────────────────────────────────────────────────────────────
static CarAlarm g_carAlarm;

// ─────────────────────────────────────────────────────────────────────────────
// Minimal system clock setup for STM32F103 @ 72 MHz (HSE 8 MHz, PLL ×9).
// Replace with your board-specific SystemInit / HAL_Init if you use CubeMX.
// ─────────────────────────────────────────────────────────────────────────────
static void systemClockConfig() {
  // Enable HSE
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY)) {
  }

  // Flash latency: 2 wait states for 72 MHz
  FLASH->ACR = FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;

  // PLL: HSE source, ×9 → 72 MHz
  RCC->CFGR = RCC_CFGR_PLLSRC        // PLL source = HSE
              | RCC_CFGR_PLLMULL9    // ×9
              | RCC_CFGR_HPRE_DIV1   // AHB  = SYSCLK / 1
              | RCC_CFGR_PPRE1_DIV2  // APB1 = AHB   / 2  (36 MHz max)
              | RCC_CFGR_PPRE2_DIV1; // APB2 = AHB   / 1

  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY)) {
  }

  // Switch SYSCLK to PLL
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
  }
}

// ─────────────────────────────────────────────────────────────────────────────
int main() {
  systemClockConfig();

  // ── Step 1: start 1 ms hardware tick (TIM2, priority 0 = highest) ─────
  auto &tm = TimerManager::instance();
  tm.init(TIM2, Config::TIM_CLK_HZ, /*nvicPriority=*/0);

  // ── Step 2: init drivers ───────────────────────────────────────────────
  //
  // TimerManager::allocate() is called here; each call gives back a unique
  // SoftwareTimer slot from the 20-slot pool.

  // Buttons — 2 timers each (debounce + double-click window)
  g_btn1.init(GPIOA, 0, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);
  g_btn2.init(GPIOA, 1, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);
  g_btn3.init(GPIOA, 2, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);

  // Motion sensor — PA7 (Rising edge = motion detected)
  g_motion.init(GPIOA, 7, ExtiPin::Trigger::Rising);

  // Relays — 1 timer each
  g_lockRelay.init(GPIOB, 0, tm.allocate());
  g_cargoRelay.init(GPIOB, 1, tm.allocate());

  // Buzzer — 1 timer
  g_buzzer.init(GPIOB, 4, tm.allocate());

  // RGB LED — R=PB5, G=PB6, B=PB7 — 1 shared timer
  g_led.init(GPIOB, 5, // R
             GPIOB, 6, // G
             GPIOB, 7, // B
             tm.allocate());

  // ── Step 3: wire everything into the application state machine ─────────
  g_carAlarm.init(&g_btn1, &g_btn2, &g_btn3, &g_motion, &g_lockRelay,
                  &g_cargoRelay, &g_buzzer, &g_led, &tm);

  // ── Step 4: idle loop — all work is done in interrupt context ──────────
  while (true) {
    __WFI(); // sleep until next interrupt (saves power)
  }
}
