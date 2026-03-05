#pragma once

#include "stm32f103x6.h"

// ─────────────────────────────────────────────────────────────────────────────
// HAL assertion and panic primitives.
//
// halPanic() — disables all interrupts, triggers a debugger breakpoint
//              (visible in any JTAG/SWD session), then spins forever.
//              Always [[noreturn]]; the compiler may elide code after any
//              call to it.
//
// HAL_ASSERT(expr) — evaluates expr once; calls halPanic() when false.
//                    Stripped to a no-op when NDEBUG is defined so release
//                    builds pay zero runtime cost.
// ─────────────────────────────────────────────────────────────────────────────

[[noreturn]] inline void halPanic() noexcept {
  __disable_irq();
  __BKPT(0);
  while (true) {
  }
}

#ifdef NDEBUG
#define HAL_ASSERT(expr) static_cast<void>(expr)
#else
#define HAL_ASSERT(expr)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      halPanic();                                                              \
    }                                                                          \
  } while (false)
#endif
