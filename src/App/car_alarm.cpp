#include "car_alarm.hpp"
#include "config.hpp"

void CarAlarm::init(Button *btn1, Button *btn2, Button *btn3,
                    MotionSensor *motion, Relay *lockRelay, Relay *cargoRelay,
                    Buzzer *buzzer, RgbLed *led, TimerManager *tm) noexcept {
  m_btn1 = btn1;
  m_btn2 = btn2;
  m_btn3 = btn3;
  m_motion = motion;
  m_lockRelay = lockRelay;
  m_cargoRelay = cargoRelay;
  m_buzzer = buzzer;
  m_led = led;

  m_preAlarmTimer = tm->allocate();
  m_preAlarmUpdate = tm->allocate();

  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);

  m_motion->setOnMotion(cbMotion, this);
  m_motion->setOnMotionEnd(cbMotionEnd, this);

  enterLocked(/*silent=*/true);
}

void CarAlarm::enterUnlocked() noexcept {
  m_state = State::Unlocked;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();
  m_buzzer->stop();
  m_motion->disable();

  m_led->setColor(false, true, false);
  m_buzzer->beep(Config::UNLOCK_BEEP_MS);
  m_lockRelay->pulse(Config::LOCK_RELAY_PULSE_MS);

  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);
}

void CarAlarm::enterLocked(bool silent) noexcept {
  m_state = State::Locked;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();
  m_buzzer->stop();

  m_led->setColor(true, false, false);

  if (!silent) {
    m_buzzer->beep(Config::LOCK_BEEP_MS);
    m_lockRelay->pulse(Config::LOCK_RELAY_PULSE_MS);
  }

  m_motion->enable();

  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);
}

void CarAlarm::enterPreAlarm() noexcept {
  m_state = State::PreAlarm;
  m_preAlarmElapsedMs = 0;
  m_motionActive = true;
  m_motionStoppedInPreAlarm = false;

  m_buzzer->stop();
  m_led->startBlink(Config::PRE_ALARM_BLINK_START_MS / 2u, true, false, false);

  // Always run to completion — outcome is decided in cbPreAlarmExpired.
  m_preAlarmTimer->start(Config::PRE_ALARM_DURATION_MS, /*oneShot=*/true,
                         cbPreAlarmExpired, this);
  m_preAlarmUpdate->start(Config::PRE_ALARM_UPDATE_STEP_MS, /*oneShot=*/false,
                          cbPreAlarmUpdate, this);
}

void CarAlarm::enterFullAlarm() noexcept {
  m_state = State::FullAlarm;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();

  m_led->startBlink(Config::ALARM_LED_HALF_PERIOD_MS, true, false, false);
  m_buzzer->playPattern(Config::BEEP_ALARM, /*repeat=*/true);

  // Any button dismisses the alarm.
  m_btn1->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn2->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn3->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn3->setOnDoubleClick(cbAnyBtnAlarmReset, this);
}

void CarAlarm::restoreLed() noexcept {
  switch (m_state) {
  case State::Unlocked:
    m_led->setColor(false, true, false);
    break;
  case State::Locked:
    m_led->setColor(true, false, false);
    break;
  default:
    break;
  }
}

void CarAlarm::onSpecialAction() noexcept {
  m_led->playFlash(Config::FLASH_BTN2, false, false, true, false, cbRestoreLed,
                   this);
  m_cargoRelay->pulse(Config::SPECIAL_ACTION_RELAY_PULSE_MS);
  m_buzzer->beep(Config::SPECIAL_ACTION_BEEP_MS);
}

void CarAlarm::updatePreAlarmBlink() noexcept {
  m_preAlarmElapsedMs += Config::PRE_ALARM_UPDATE_STEP_MS;

  const uint32_t elapsed = (m_preAlarmElapsedMs < Config::PRE_ALARM_DURATION_MS)
                               ? m_preAlarmElapsedMs
                               : Config::PRE_ALARM_DURATION_MS;
  const uint32_t total = Config::PRE_ALARM_DURATION_MS;
  const uint32_t startMs = Config::PRE_ALARM_BLINK_START_MS;
  const uint32_t endMs = Config::PRE_ALARM_BLINK_END_MS;

  // Linear interpolation: period shrinks from startMs → endMs as elapsed →
  // total.
  const uint32_t period = startMs - (startMs - endMs) * elapsed / total;
  m_led->setBlinkPeriod(period / 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static callback trampolines
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::cbBtn1Click(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  switch (self->m_state) {
  case State::Unlocked:
    self->enterLocked();
    break;
  case State::Locked:
  case State::PreAlarm:
    self->enterUnlocked();
    break;
  default:
    break;
  }
}

void CarAlarm::cbBtn2Click(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::Unlocked || self->m_state == State::Locked)
    self->onSpecialAction();
}

void CarAlarm::cbBtn3DoubleClick(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state != State::Unlocked && self->m_state != State::Locked)
    return;
  self->m_cargoRelay->pulse(Config::CARGO_RELAY_PULSE_MS);
  self->m_led->playFlash(Config::FLASH_CARGO, true, false, true, false,
                         cbRestoreLed, self);
  self->m_buzzer->playPattern(Config::BEEP_CARGO);
}

// ── Motion started
// ────────────────────────────────────────────────────────────
//  Locked   → start the pre-alarm countdown.
//  PreAlarm → motion stopped then restarted: escalate to FullAlarm immediately.

void CarAlarm::cbMotion(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  switch (self->m_state) {
  case State::Locked:
    self->enterPreAlarm();
    break;
  case State::PreAlarm:
    self->m_motionActive = true;
    if (self->m_motionStoppedInPreAlarm) {
      // Disappeared then reappeared inside the 5 s window → alarm now.
      self->m_preAlarmTimer->stop();
      self->m_preAlarmUpdate->stop();
      self->enterFullAlarm();
    }
    break;
  default:
    break;
  }
}

// ── Motion stopped
// ────────────────────────────────────────────────────────────
//  PreAlarm → record that motion has gone.  Do NOT touch the timer — the 5 s
//  blink always runs to completion.  cbPreAlarmExpired checks m_motionActive
//  and returns to Locked if motion is still absent when the countdown finishes.

void CarAlarm::cbMotionEnd(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::PreAlarm) {
    self->m_motionActive = false;
    self->m_motionStoppedInPreAlarm = true;
  }
}

// ── Pre-alarm timer expired
// ───────────────────────────────────────────────────
//  Motion still present → FullAlarm.
//  Motion gone          → return to Locked quietly (alarm never triggered).

void CarAlarm::cbPreAlarmExpired(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  self->m_preAlarmUpdate->stop();
  if (self->m_motionActive)
    self->enterFullAlarm();
  else
    self->enterLocked(/*silent=*/true); // no beep / relay — alarm never fired
}

void CarAlarm::cbAnyBtnAlarmReset(void *ctx) noexcept {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::FullAlarm)
    self->enterLocked(/*silent=*/true); // dismissing alarm: no re-lock signal
}

void CarAlarm::cbPreAlarmUpdate(void *ctx) noexcept {
  static_cast<CarAlarm *>(ctx)->updatePreAlarmBlink();
}

void CarAlarm::cbRestoreLed(void *ctx) noexcept {
  static_cast<CarAlarm *>(ctx)->restoreLed();
}
