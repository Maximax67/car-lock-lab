#pragma once

#include "Drivers/button.hpp"
#include "Drivers/buzzer.hpp"
#include "Drivers/motion_sensor.hpp"
#include "Drivers/relay.hpp"
#include "Drivers/rgb_led.hpp"
#include "HAL/Timer/software_timer.hpp"
#include "HAL/Timer/timer_manager.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// CarAlarm — application-layer state machine.
//
// States
// ──────
//  Unlocked  : Green LED solid.  Lock button locks; cargo button opens cargo.
//  Locked    : Red LED solid.    Unlock button unlocks; motion → PreAlarm.
//  PreAlarm  : Red LED blinking (1 s period → 200 ms over 5 s).
//              Second motion trigger → FullAlarm immediately.
//              Pre-alarm timer expires → FullAlarm.
//  FullAlarm : Red LED blinking at 125 ms period.  Buzzer 300 ms period.
//              Any button press → back to Locked.
//
// Cargo door (double-click button 3) works in Unlocked and Locked states.
//
// Button 2 (PA1) fires onSpecialAction() which is intentionally left as a
// no-op override point — replace with your custom logic.
// ─────────────────────────────────────────────────────────────────────────────
class CarAlarm {
public:
  enum class State { Unlocked, Locked, PreAlarm, FullAlarm };

  // Wire all hardware.  Call once from main() after init'ing TimerManager.
  void init(Button *btn1, // lock / unlock
            Button *btn2, // special action
            Button *btn3, // cargo (double-click)
            MotionSensor *motion, Relay *lockRelay, Relay *cargoRelay,
            Buzzer *buzzer, RgbLed *led, TimerManager *tm);

  State state() const { return m_state; }

protected:
  // Override point for button 2 (special action).
  virtual void onSpecialAction() {}

private:
  // ── State transitions ─────────────────────────────────────────────────
  void enterUnlocked();
  void enterLocked();
  void enterPreAlarm();
  void enterFullAlarm();

  // ── Pre-alarm helpers ─────────────────────────────────────────────────
  void updatePreAlarmBlink();

  // ── Static callback trampolines ───────────────────────────────────────
  static void cbBtn1Click(void *ctx);
  static void cbBtn2Click(void *ctx);
  static void cbBtn3DoubleClick(void *ctx);
  static void cbMotion(void *ctx);
  static void cbAnyBtnAlarmReset(void *ctx); // buttons reset FullAlarm
  static void cbPreAlarmExpired(void *ctx);
  static void cbPreAlarmUpdate(void *ctx);

  // ── Hardware references (not owned) ──────────────────────────────────
  Button *m_btn1 = nullptr;
  Button *m_btn2 = nullptr;
  Button *m_btn3 = nullptr;
  MotionSensor *m_motion = nullptr;
  Relay *m_lockRelay = nullptr;
  Relay *m_cargoRelay = nullptr;
  Buzzer *m_buzzer = nullptr;
  RgbLed *m_led = nullptr;

  // ── Timers (allocated from TimerManager pool) ─────────────────────────
  SoftwareTimer *m_preAlarmTimer = nullptr;  // 5 s one-shot
  SoftwareTimer *m_preAlarmUpdate = nullptr; // 100 ms repeating interpolator

  // ── Runtime state ─────────────────────────────────────────────────────
  State m_state = State::Unlocked;
  uint32_t m_preAlarmElapsedMs = 0;
  bool m_motionSeenInPreAlarm = false; // true once first motion seen
};
