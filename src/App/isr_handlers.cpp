#include "HAL/exti.hpp"
#include "HAL/Timer/timer_manager.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// ISR Handlers — the only file that knows about both the CMSIS vector names
// and the HAL dispatch API.  Keep this file thin: one line per ISR.
//
// Pin → ISR mapping for this project
// ───────────────────────────────────
//  PA0  Button 1       → EXTI0
//  PA1  Button 2       → EXTI1
//  PA2  Button 3       → EXTI2
//  PA7  Motion sensor  → EXTI9_5 (lines 5-9 share one vector)
//  TIM2 1 ms tick      → TIM2_IRQHandler
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {

// ── 1 ms hardware tick ───────────────────────────────────────────────────────
void TIM2_IRQHandler() { TimerManager::instance().handleTimerIrq(); }

// ── Button 1 (PA0) ───────────────────────────────────────────────────────────
void EXTI0_IRQHandler() { ExtiPin::dispatch(0); }

// ── Button 2 (PA1) ───────────────────────────────────────────────────────────
void EXTI1_IRQHandler() { ExtiPin::dispatch(1); }

// ── Button 3 (PA2) ───────────────────────────────────────────────────────────
void EXTI2_IRQHandler() { ExtiPin::dispatch(2); }

// ── Motion sensor (PA7) — shares EXTI9_5 vector ──────────────────────────────
void EXTI9_5_IRQHandler() { ExtiPin::dispatchRange(5, 9); }

} // extern "C"