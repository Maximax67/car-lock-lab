#pragma once

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// SoftwareTimer — a logical countdown/repeating timer driven by TimerManager.
//
// All instances live in TimerManager's pool (no heap). Obtain one via
// TimerManager::instance().allocate(), then call init on the owner object.
//
// Thread-safety: start() and stop() wrap their writes in __disable_irq /
// __enable_irq because EXTI callbacks (priority 1) can call them while
// TIM2 (priority 0) can preempt EXTI. The guard prevents a torn write.
// ─────────────────────────────────────────────────────────────────────────────
class SoftwareTimer {
public:
  using Callback = void (*)(void *ctx);

  // Start or restart the timer.
  // intervalMs : period in milliseconds
  // oneShot    : fire once then stop (false = repeating)
  // cb / ctx   : optional callback + context pointer
  void start(uint32_t intervalMs, bool oneShot = true, Callback cb = nullptr,
             void *ctx = nullptr);

  void stop();

  // Resets elapsed counter; keeps all other settings intact.
  // Useful for debounce "restart on noise".
  void restart();

  bool isRunning() const { return m_running; }
  uint32_t elapsed() const { return m_elapsed; }

  // ── Called only from TimerManager::onHwTick() ─────────────────────────
  void _tick();

private:
  uint32_t m_interval = 0;
  uint32_t m_elapsed = 0;
  bool m_running = false;
  bool m_oneShot = false;
  Callback m_cb = nullptr;
  void *m_ctx = nullptr;
};
