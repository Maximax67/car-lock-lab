#pragma once

#include "Drivers/button.hpp"
#include "Drivers/buzzer.hpp"
#include "Drivers/motion_sensor.hpp"
#include "Drivers/relay.hpp"
#include "Drivers/rgb_led.hpp"
#include "HAL/Timer/software_timer.hpp"
#include "HAL/Timer/timer_manager.hpp"

class CarAlarm {
public:
  enum class State { Unlocked, Locked, PreAlarm, FullAlarm };

  virtual ~CarAlarm() = default;

  void init(Button *btn1, Button *btn2, Button *btn3, MotionSensor *motion,
            Relay *lockRelay, Relay *cargoRelay, Buzzer *buzzer, RgbLed *led,
            TimerManager *tm) noexcept;

  [[nodiscard]] State state() const noexcept { return m_state; }

protected:
  virtual void onSpecialAction() noexcept;

private:
  void enterUnlocked() noexcept;

  // silent=true  — returning from PreAlarm / FullAlarm: no beep, no relay.
  // silent=false — user-initiated lock: beep + relay pulse as normal.
  void enterLocked(bool silent = false) noexcept;
  void enterPreAlarm() noexcept;
  void enterFullAlarm() noexcept;

  void restoreLed() noexcept;
  void updatePreAlarmBlink() noexcept;

  static void cbBtn1Click(void *ctx) noexcept;
  static void cbBtn2Click(void *ctx) noexcept;
  static void cbBtn3DoubleClick(void *ctx) noexcept;
  static void cbMotion(void *ctx) noexcept;    // sensor HIGH — motion started
  static void cbMotionEnd(void *ctx) noexcept; // sensor LOW  — motion stopped
  static void cbAnyBtnAlarmReset(void *ctx) noexcept;
  static void cbPreAlarmExpired(void *ctx) noexcept;
  static void cbPreAlarmUpdate(void *ctx) noexcept;
  static void cbRestoreLed(void *ctx) noexcept;

  Button *m_btn1 = nullptr;
  Button *m_btn2 = nullptr;
  Button *m_btn3 = nullptr;
  MotionSensor *m_motion = nullptr;
  Relay *m_lockRelay = nullptr;
  Relay *m_cargoRelay = nullptr;
  Buzzer *m_buzzer = nullptr;
  RgbLed *m_led = nullptr;

  SoftwareTimer *m_preAlarmTimer = nullptr;
  SoftwareTimer *m_preAlarmUpdate = nullptr;

  State m_state = State::Unlocked;
  uint32_t m_preAlarmElapsedMs = 0;

  // True while the motion sensor is actively reporting motion.
  // Set on entering PreAlarm (motion was the trigger) and updated live by
  // cbMotion / cbMotionEnd.  Read by cbPreAlarmExpired to decide outcome.
  bool m_motionActive = false;

  // Latched true the first time motion stops during PreAlarm.
  // If motion restarts before the 5 s timer expires we escalate to
  // FullAlarm immediately rather than waiting for the countdown.
  bool m_motionStoppedInPreAlarm = false;
};
