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
//  PreAlarm  : Red LED blinking (1 s → 200 ms period over 5 s).
//              The 5 s countdown always runs to completion.
//              • Motion gone, 5 s expires          → back to Locked (no alarm).
//              • Motion present when 5 s expires   → FullAlarm.
//              • Motion stops then restarts < 5 s  → FullAlarm immediately.
//  FullAlarm : Red LED 125 ms blink.  Buzzer 300 ms period.
//              Any button press → back to Locked.
//
// Cargo door (double-click button 3) works in Unlocked and Locked states.
// Button 2 triggers a 300 ms blue LED flash.
// Override onSpecialAction() in a subclass to replace this behaviour.
// ─────────────────────────────────────────────────────────────────────────────
class CarAlarm {
public:
  enum class State { Unlocked, Locked, PreAlarm, FullAlarm };

  void init(Button *btn1, Button *btn2, Button *btn3, MotionSensor *motion,
            Relay *lockRelay, Relay *cargoRelay, Buzzer *buzzer, RgbLed *led,
            TimerManager *tm);

  State state() const { return m_state; }

protected:
  virtual void onSpecialAction();

private:
  void enterUnlocked();
  // silent=true suppresses the beep and relay pulse.
  // Used when returning to Locked from PreAlarm or FullAlarm — those
  // transitions are not user-initiated lock events.
  void enterLocked(bool silent = false);
  void enterPreAlarm();
  void enterFullAlarm();

  void restoreLed();
  void updatePreAlarmBlink();

  static void cbBtn1Click(void *ctx);
  static void cbBtn2Click(void *ctx);
  static void cbBtn3DoubleClick(void *ctx);
  static void cbMotion(void *ctx);    // sensor HIGH — motion started
  static void cbMotionEnd(void *ctx); // sensor LOW  — motion stopped
  static void cbAnyBtnAlarmReset(void *ctx);
  static void cbPreAlarmExpired(void *ctx);
  static void cbPreAlarmUpdate(void *ctx);
  static void cbRestoreLed(void *ctx);

  Button *m_btn1 = nullptr;
  Button *m_btn2 = nullptr;
  Button *m_btn3 = nullptr;
  MotionSensor *m_motion = nullptr;
  Relay *m_lockRelay = nullptr;
  Relay *m_cargoRelay = nullptr;
  Buzzer *m_buzzer = nullptr;
  RgbLed *m_led = nullptr;

  SoftwareTimer *m_preAlarmTimer = nullptr;  // 5 s one-shot
  SoftwareTimer *m_preAlarmUpdate = nullptr; // 100 ms repeating interpolator

  State m_state = State::Unlocked;
  uint32_t m_preAlarmElapsedMs = 0;

  // True while the motion sensor is actively reporting motion.
  // Set on entering PreAlarm (motion was the trigger) and updated by
  // cbMotion / cbMotionEnd.  Read by cbPreAlarmExpired to decide outcome.
  bool m_motionActive = false;

  // Latched true the first time motion stops during PreAlarm.
  // If motion then restarts before the 5 s timer expires we escalate
  // to FullAlarm immediately instead of waiting for the countdown.
  bool m_motionStoppedInPreAlarm = false;
};
