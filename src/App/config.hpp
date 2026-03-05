#pragma once

#include "Drivers/buzzer.hpp"
#include "Drivers/rgb_led.hpp"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Config — single source of truth for every timing constant and hardware
// pattern.  Change values here; nothing else needs touching.
// ─────────────────────────────────────────────────────────────────────────────
namespace Config {

// ── Hardware clock ──────────────────────────────────────────────────────────
constexpr uint32_t TIM_CLK_HZ = 8'000'000u;

// ── Button timing ────────────────────────────────────────────────────────────
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_DOUBLE_CLICK_MS = 1000;

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
constexpr uint32_t PRE_ALARM_BLINK_START_MS = 1000; // full period at start
constexpr uint32_t PRE_ALARM_BLINK_END_MS = 200;    // full period at end
constexpr uint32_t PRE_ALARM_UPDATE_STEP_MS = 100;  // interpolation granularity

// ── Full alarm
// ────────────────────────────────────────────────────────────────
constexpr uint32_t ALARM_LED_HALF_PERIOD_MS = 62; // ~125 ms full period
constexpr uint32_t ALARM_BEEP_HALF_PERIOD = 150;  // 300 ms full period

// ── Pattern element counts
// ────────────────────────────────────────────────────
constexpr uint8_t BEEP_UNLOCK_COUNT = 1;
constexpr uint8_t BEEP_LOCK_COUNT = 1;
constexpr uint8_t BEEP_CARGO_COUNT = 2;
constexpr uint8_t BEEP_ALARM_COUNT = 1;
constexpr uint8_t FLASH_CARGO_COUNT = 3;

// ── Buzzer patterns (defined in config.cpp)
// ───────────────────────────────────
extern const BeepPattern BEEP_UNLOCK[BEEP_UNLOCK_COUNT]; // 150 ms
extern const BeepPattern BEEP_LOCK[BEEP_LOCK_COUNT];     // 400 ms
extern const BeepPattern BEEP_CARGO[BEEP_CARGO_COUNT];   // 2× 100 ms
extern const BeepPattern BEEP_ALARM[BEEP_ALARM_COUNT];   // repeating

// ── LED flash patterns (defined in config.cpp)
// ────────────────────────────────
extern const FlashStep FLASH_CARGO[FLASH_CARGO_COUNT]; // 3× 100 ms

} // namespace Config
