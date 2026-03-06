#pragma once

#include <cstdint>

class TimerManager; // forward declaration for friend

class SoftwareTimer {
  friend class TimerManager;

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

  // Reset elapsed to zero
  void restart() noexcept;

  [[nodiscard]] bool isRunning() const noexcept { return m_running; }
  [[nodiscard]] uint32_t elapsed() const noexcept { return m_elapsed; }

private:
  void tick() noexcept;

  uint32_t m_interval = 0;
  Callback m_cb = nullptr;
  void *m_ctx = nullptr;
  bool m_oneShot = false;

  volatile bool m_running = false;
  volatile uint32_t m_elapsed = 0;
};
