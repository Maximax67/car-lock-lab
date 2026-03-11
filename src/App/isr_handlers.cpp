#include "Drivers/rgb_led.hpp"
#include "HAL/Timer/timer_manager.hpp"
#include "HAL/exti.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  SysTick   1 ms tick          → SysTick_Handler
//  TIM2      LED blink toggle   → TIM2_IRQHandler
//
//  PA0  Button 1                → EXTI0_IRQHandler
//  PA1  Button 2                → EXTI1_IRQHandler
//  PA2  Button 3                → EXTI2_IRQHandler
//  PA7  Motion sensor           → EXTI9_5_IRQHandler
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {
void SysTick_Handler() { TimerManager::instance().handleTimerIrq(); }
void TIM2_IRQHandler() { RgbLed::dispatchBlinkIrq(); }

void EXTI0_IRQHandler() { ExtiPin::dispatch(0); }
void EXTI1_IRQHandler() { ExtiPin::dispatch(1); }
void EXTI2_IRQHandler() { ExtiPin::dispatch(2); }
void EXTI9_5_IRQHandler() { ExtiPin::dispatchRange(5, 9); }
}
