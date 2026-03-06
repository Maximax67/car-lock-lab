#include "HAL/Timer/timer_manager.hpp"
#include "HAL/exti.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Pin → ISR mapping
// ─────────────────
//  PA0  Button 1       → EXTI0
//  PA1  Button 2       → EXTI1
//  PA2  Button 3       → EXTI2
//  PA7  Motion sensor  → EXTI9_5
//  TIM2 1 ms tick      → TIM2_IRQHandler
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {
void TIM2_IRQHandler() { TimerManager::instance().handleTimerIrq(); }
void EXTI0_IRQHandler() { ExtiPin::dispatch(0); }
void EXTI1_IRQHandler() { ExtiPin::dispatch(1); }
void EXTI2_IRQHandler() { ExtiPin::dispatch(2); }
void EXTI9_5_IRQHandler() { ExtiPin::dispatchRange(5, 9); }
}
