#include "config.hpp"

namespace Config {

// Single 150 ms beep.
const BeepPattern BEEP_UNLOCK[BEEP_UNLOCK_COUNT] = {{UNLOCK_BEEP_MS, 0}};

// Single 400 ms beep.
const BeepPattern BEEP_LOCK[BEEP_LOCK_COUNT] = {{LOCK_BEEP_MS, 0}};

// Two 100 ms beeps separated by a 100 ms gap.
const BeepPattern BEEP_CARGO[BEEP_CARGO_COUNT] = {
    {100, 100}, // first beep + gap
    {100, 0},   // second beep, no trailing gap
};

// Alarm: 150 ms on / 150 ms off, repeating (playPattern(..., repeat=true)).
const BeepPattern BEEP_ALARM[BEEP_ALARM_COUNT] = {
    {ALARM_BEEP_HALF_PERIOD, ALARM_BEEP_HALF_PERIOD},
};

// Cargo LED: 3 flashes — 100 ms on, 100 ms off each.
const FlashStep FLASH_CARGO[FLASH_CARGO_COUNT] = {
    {100, 100},
    {100, 100},
    {100, 0}, // last flash — no trailing off needed
};

// Button 2 LED: single 300 ms blue flash.
const FlashStep FLASH_BTN2[FLASH_BTN2_COUNT] = {
    {300, 0},
};

} // namespace Config
