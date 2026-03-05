#pragma once

#include <cstdint>

class TimerManager; // forward declaration for friend

// ─────────────────────────────────────────────────────────────────────────────
// SoftwareTimer — a logical countdown / repeating timer driven by TimerManager.
//
// All instances live in TimerManager's fixed-size pool (no heap).  Obtain
// one via TimerManager::instance().allocate(), then pass it to a driver's
// init() function.
//
// Thread-safety model
// ───────────────────
// _tick() runs exclusively inside the TIM2 ISR (priority 0).
// start() and stop() may be called from EXTI ISR context (priority 1) or
// from main.  They wrap all writes in __disable_irq / __enable_irq to
// prevent a torn read in the TIM2 ISR.
//
// m_running and m_elapsed are declared volatile because they are written by
// the TIM2 ISR and read by callers of isRunning() / elapsed() without an
// IRQ guard — the compiler must not cache them in a register.
// ─────────────────────────────────────────────────────────────────────────────
class SoftwareTimer {
  friend class TimerManager; // only TimerManager may call tick()

public:
  using Callback = void (*)(void *ctx) noexcept;

  // Start or restart the timer.
  // intervalMs : period in milliseconds (must be > 0)
  // oneShot    : fire once then stop; false = repeating
  // cb / ctx   : callback + opaque context pointer
  void start(uint32_t intervalMs, bool oneShot = true, Callback cb = nullptr,
             void *ctx = nullptr) noexcept;

  // Stop the timer and reset its elapsed counter.
  void stop() noexcept;

  // Reset elapsed to zero without changing any other setting.
  // Useful for debounce "extend on noise" patterns.
  void restart() noexcept;

  [[nodiscard]] bool isRunning() const noexcept { return m_running; }
  [[nodiscard]] uint32_t elapsed() const noexcept { return m_elapsed; }

private:
  // Called only by TimerManager::onHwTick() — advances the countdown by 1 ms
  // and fires the callback when the interval is reached.  Kept private to
  // prevent accidental direct calls; TimerManager is the sole caller.
  void tick() noexcept;

  uint32_t m_interval = 0;
  Callback m_cb = nullptr;
  void *m_ctx = nullptr;
  bool m_oneShot = false;

  // Written by the TIM2 ISR; read from main/EXTI without a critical section.
  volatile bool m_running = false;
  volatile uint32_t m_elapsed = 0;
};
