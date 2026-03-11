#pragma once

#include "software_timer.hpp"
#include "systick_timer.hpp"
#include <array>
#include <cstdint>

constexpr uint8_t MAX_SW_TIMERS = 20;

class TimerManager {
public:
  [[nodiscard]] static TimerManager &instance() noexcept;

  void init(uint32_t coreClkHz, uint8_t nvicPriority = 0) noexcept;

  [[nodiscard]] SoftwareTimer *allocate() noexcept;

  void handleTimerIrq() noexcept;

  [[nodiscard]] uint32_t ticks() const noexcept { return m_ticks; }

private:
  TimerManager() = default;
  TimerManager(const TimerManager &) = delete;
  TimerManager &operator=(const TimerManager &) = delete;

  static void onHwTick(void *ctx) noexcept;

  SysTickTimer m_sysTick;
  std::array<SoftwareTimer, MAX_SW_TIMERS> m_pool{};
  std::array<bool, MAX_SW_TIMERS> m_used{};
  volatile uint32_t m_ticks = 0;
};
