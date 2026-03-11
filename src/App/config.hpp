#pragma once

#include "Drivers/buzzer.hpp"
#include "Drivers/rgb_led.hpp"
#include "stm32f103x6.h"
#include <array>
#include <cstdint>

namespace Config {

constexpr uint32_t CORE_CLK_HZ = 8'000'000u;
constexpr uint8_t SYSTICK_NVIC_PRIORITY = 0;

// ---------------------------------------------------------------------------
// RGB LED blink timer (TIM2)
// ---------------------------------------------------------------------------
inline TIM_TypeDef *const LED_BLINK_TIM = TIM2;
constexpr uint32_t LED_BLINK_TIM_CLK_HZ = CORE_CLK_HZ;
constexpr uint8_t LED_BLINK_TIM_NVIC_PRIORITY = 2;

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 500;

inline GPIO_TypeDef *const BTN1_PORT = GPIOA;
constexpr uint8_t BTN1_PIN = 0;

inline GPIO_TypeDef *const BTN2_PORT = GPIOA;
constexpr uint8_t BTN2_PIN = 1;

inline GPIO_TypeDef *const BTN3_PORT = GPIOA;
constexpr uint8_t BTN3_PIN = 2;

// ---------------------------------------------------------------------------
// Motion sensor
// ---------------------------------------------------------------------------
constexpr uint32_t MOTION_DEBOUNCE_MS = 50;

inline GPIO_TypeDef *const MOTION_PORT = GPIOA;
constexpr uint8_t MOTION_PIN = 7;
constexpr auto MOTION_TRIGGER = ExtiPin::Trigger::Both;

// ---------------------------------------------------------------------------
// Relays
// ---------------------------------------------------------------------------
constexpr uint32_t LOCK_RELAY_PULSE_MS = 250;
constexpr uint32_t CARGO_RELAY_PULSE_MS = 300;

inline GPIO_TypeDef *const LOCK_RELAY_PORT = GPIOB;
constexpr uint8_t LOCK_RELAY_PIN = 0;

inline GPIO_TypeDef *const CARGO_RELAY_PORT = GPIOB;
constexpr uint8_t CARGO_RELAY_PIN = 1;

// ---------------------------------------------------------------------------
// Buzzer
// ---------------------------------------------------------------------------
constexpr uint32_t UNLOCK_BEEP_MS = 150;
constexpr uint32_t LOCK_BEEP_MS = 400;

inline GPIO_TypeDef *const BUZZER_PORT = GPIOB;
constexpr uint8_t BUZZER_PIN = 4;

// ---------------------------------------------------------------------------
// RGB LED
// ---------------------------------------------------------------------------
inline GPIO_TypeDef *const LED_R_PORT = GPIOB;
constexpr uint8_t LED_R_PIN = 5;

inline GPIO_TypeDef *const LED_G_PORT = GPIOB;
constexpr uint8_t LED_G_PIN = 6;

inline GPIO_TypeDef *const LED_B_PORT = GPIOB;
constexpr uint8_t LED_B_PIN = 7;

// ---------------------------------------------------------------------------
// Pre-alarm
// ---------------------------------------------------------------------------
constexpr uint32_t PRE_ALARM_DURATION_MS = 5000;
constexpr uint32_t PRE_ALARM_BLINK_START_MS = 1000;
constexpr uint32_t PRE_ALARM_BLINK_END_MS = 200;
constexpr uint32_t PRE_ALARM_UPDATE_STEP_MS = 100;

// ---------------------------------------------------------------------------
// Alarm
// ---------------------------------------------------------------------------
constexpr uint32_t ALARM_LED_HALF_PERIOD_MS = 62;
constexpr uint32_t ALARM_BEEP_HALF_PERIOD = 150;

inline constexpr std::array<BeepPattern, 1> BEEP_ALARM = {{
    {ALARM_BEEP_HALF_PERIOD, ALARM_BEEP_HALF_PERIOD},
}};

// ---------------------------------------------------------------------------
// Cargo
// ---------------------------------------------------------------------------
inline constexpr std::array<BeepPattern, 2> BEEP_CARGO = {{
    {100u, 100u},
    {100u, 0u},
}};

inline constexpr std::array<FlashStep, 3> FLASH_CARGO = {{
    {100u, 100u},
    {100u, 100u},
    {100u, 0u},
}};

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------
inline constexpr std::array<FlashStep, 1> FLASH_BTN2 = {{
    {300u, 0u},
}};

constexpr uint32_t SPECIAL_ACTION_RELAY_PULSE_MS = 300;
constexpr uint32_t SPECIAL_ACTION_BEEP_MS = 300;

} // namespace Config
