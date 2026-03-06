#pragma once

#include "Drivers/buzzer.hpp"
#include "Drivers/rgb_led.hpp"
#include <array>
#include <cstdint>

namespace Config {
constexpr uint32_t TIM_CLK_HZ = 8'000'000u;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 500;

constexpr uint32_t MOTION_DEBOUNCE_MS = 50;

constexpr uint32_t UNLOCK_BEEP_MS = 150;
constexpr uint32_t LOCK_BEEP_MS = 400;
constexpr uint32_t LOCK_RELAY_PULSE_MS = 250;

constexpr uint32_t CARGO_RELAY_PULSE_MS = 300;

constexpr uint32_t PRE_ALARM_DURATION_MS = 5000;    // total pre-alarm window
constexpr uint32_t PRE_ALARM_BLINK_START_MS = 1000; // full blink period at t=0
constexpr uint32_t PRE_ALARM_BLINK_END_MS = 200;   // full blink period at t=end
constexpr uint32_t PRE_ALARM_UPDATE_STEP_MS = 100; // interpolation granularity

constexpr uint32_t ALARM_LED_HALF_PERIOD_MS = 62; // ~125 ms full period
constexpr uint32_t ALARM_BEEP_HALF_PERIOD = 150;  // 300 ms full period

inline constexpr std::array<BeepPattern, 1> BEEP_ALARM = {{
    {ALARM_BEEP_HALF_PERIOD, ALARM_BEEP_HALF_PERIOD},
}};

inline constexpr std::array<BeepPattern, 2> BEEP_CARGO = {{
    {100u, 100u},
    {100u, 0u},
}};

inline constexpr std::array<FlashStep, 3> FLASH_CARGO = {{
    {100u, 100u},
    {100u, 100u},
    {100u, 0u},
}};

inline constexpr std::array<FlashStep, 1> FLASH_BTN2 = {{
    {300u, 0u},
}};

constexpr uint32_t SPECIAL_ACTION_RELAY_PULSE_MS = 300;
constexpr uint32_t SPECIAL_ACTION_BEEP_MS = 300;
}
