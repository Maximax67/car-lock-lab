#pragma once

#include "Drivers/buzzer.hpp"
#include "Drivers/rgb_led.hpp"
#include <array>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Config — single source of truth for every timing constant and hardware
// pattern.  All values are constexpr; no config.cpp is required.
//
// Change values here; nothing else needs touching.
// ─────────────────────────────────────────────────────────────────────────────
namespace Config {

// ── Hardware clock
// ────────────────────────────────────────────────────────────
constexpr uint32_t TIM_CLK_HZ = 8'000'000u;

// ── Button timing
// ─────────────────────────────────────────────────────────────
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 500;

// ── Motion sensor timing
// ────────────────────────────────────────────────────── Debounce window for
// the PIR edge.  Long enough to reject glitches, short enough to not miss a
// real event (PIR outputs are stable for 2+ seconds).
constexpr uint32_t MOTION_DEBOUNCE_MS = 50;

// ── Lock / Unlock
// ─────────────────────────────────────────────────────────────
constexpr uint32_t UNLOCK_BEEP_MS = 150;
constexpr uint32_t LOCK_BEEP_MS = 400;
constexpr uint32_t LOCK_RELAY_PULSE_MS = 250;

// ── Cargo door
// ────────────────────────────────────────────────────────────────
constexpr uint32_t CARGO_RELAY_PULSE_MS = 300;

// ── Pre-alarm (motion detected while locked)
// ──────────────────────────────────
constexpr uint32_t PRE_ALARM_DURATION_MS = 5000;    // total pre-alarm window
constexpr uint32_t PRE_ALARM_BLINK_START_MS = 1000; // full blink period at t=0
constexpr uint32_t PRE_ALARM_BLINK_END_MS = 200;   // full blink period at t=end
constexpr uint32_t PRE_ALARM_UPDATE_STEP_MS = 100; // interpolation granularity

// ── Full alarm
// ────────────────────────────────────────────────────────────────
constexpr uint32_t ALARM_LED_HALF_PERIOD_MS = 62; // ~125 ms full period
constexpr uint32_t ALARM_BEEP_HALF_PERIOD = 150;  // 300 ms full period

// ── Buzzer patterns
// ───────────────────────────────────────────────────────────

// Alarm: 150 ms on / 150 ms off, used with repeat=true.
inline constexpr std::array<BeepPattern, 1> BEEP_ALARM = {{
    {ALARM_BEEP_HALF_PERIOD, ALARM_BEEP_HALF_PERIOD},
}};

// Cargo: two 100 ms beeps separated by a 100 ms gap.
inline constexpr std::array<BeepPattern, 2> BEEP_CARGO = {{
    {100u, 100u}, // first beep + gap
    {100u, 0u},   // second beep, no trailing gap
}};

// ── LED flash patterns
// ────────────────────────────────────────────────────────

// Cargo: 3× 100 ms red+blue flashes with 100 ms gaps.
inline constexpr std::array<FlashStep, 3> FLASH_CARGO = {{
    {100u, 100u},
    {100u, 100u},
    {100u, 0u}, // last flash — no trailing off needed
}};

// Button 2 action: single 300 ms blue flash.
inline constexpr std::array<FlashStep, 1> FLASH_BTN2 = {{
    {300u, 0u},
}};

constexpr uint32_t SPECIAL_ACTION_RELAY_PULSE_MS = 300;
constexpr uint32_t SPECIAL_ACTION_BEEP_MS = 300;

} // namespace Config
