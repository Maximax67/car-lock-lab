#include "car_alarm.hpp"
#include "config.hpp"

// ── init
// ──────────────────────────────────────────────────────────────────────

void CarAlarm::init(Button *btn1, Button *btn2, Button *btn3,
                    MotionSensor *motion, Relay *lockRelay, Relay *cargoRelay,
                    Buzzer *buzzer, RgbLed *led, TimerManager *tm) {
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

  // Startup: go to Locked silently — no relay pulse or beep on power-on.
  enterLocked(/*silent=*/true);
}

// ─────────────────────────────────────────────────────────────────────────────
// State transitions
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::enterUnlocked() {
  m_state = State::Unlocked;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();
  m_buzzer->stop();
  m_motion->disable();

  m_led->setColor(false, true, false); // solid green
  m_buzzer->beep(Config::UNLOCK_BEEP_MS);
  m_lockRelay->pulse(Config::LOCK_RELAY_PULSE_MS);

  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);
}

// silent=true  — return to Locked from PreAlarm/FullAlarm: no beep, no relay.
// silent=false — explicit user lock: beep + relay pulse as normal.
void CarAlarm::enterLocked(bool silent) {
  m_state = State::Locked;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();
  m_buzzer->stop();

  m_led->setColor(true, false, false); // solid red

  if (!silent) {
    m_buzzer->beep(Config::LOCK_BEEP_MS);
    m_lockRelay->pulse(Config::LOCK_RELAY_PULSE_MS);
  }

  m_motion->enable();

  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);
}

void CarAlarm::enterPreAlarm() {
  m_state = State::PreAlarm;
  m_preAlarmElapsedMs = 0;
  m_motionActive = true; // motion is present — it triggered this
  m_motionStoppedInPreAlarm = false;

  m_buzzer->stop();

  m_led->startBlink(Config::PRE_ALARM_BLINK_START_MS / 2, true, false, false);

  // Always run to completion — outcome decided in cbPreAlarmExpired.
  m_preAlarmTimer->start(Config::PRE_ALARM_DURATION_MS, /*oneShot=*/true,
                         cbPreAlarmExpired, this);
  m_preAlarmUpdate->start(Config::PRE_ALARM_UPDATE_STEP_MS, /*oneShot=*/false,
                          cbPreAlarmUpdate, this);
}

void CarAlarm::enterFullAlarm() {
  m_state = State::FullAlarm;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();

  m_led->startBlink(Config::ALARM_LED_HALF_PERIOD_MS, true, false, false);
  m_buzzer->playPattern(Config::BEEP_ALARM, Config::BEEP_ALARM_COUNT,
                        /*repeat=*/true);

  m_btn1->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn2->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn3->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn3->setOnDoubleClick(cbAnyBtnAlarmReset, this);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::restoreLed() {
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

void CarAlarm::onSpecialAction() {
  m_led->playFlash(Config::FLASH_BTN2, Config::FLASH_BTN2_COUNT, false, false,
                   true, false, cbRestoreLed, this);
}

void CarAlarm::updatePreAlarmBlink() {
  m_preAlarmElapsedMs += Config::PRE_ALARM_UPDATE_STEP_MS;

  uint32_t elapsed = m_preAlarmElapsedMs;
  uint32_t total = Config::PRE_ALARM_DURATION_MS;
  uint32_t startMs = Config::PRE_ALARM_BLINK_START_MS;
  uint32_t endMs = Config::PRE_ALARM_BLINK_END_MS;

  if (elapsed > total)
    elapsed = total;

  uint32_t period = startMs - (startMs - endMs) * elapsed / total;
  m_led->setBlinkPeriod(period / 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static callback trampolines
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::cbBtn1Click(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  switch (self->m_state) {
  case State::Unlocked:
    self->enterLocked(); // user-initiated: silent=false (default)
    break;
  case State::Locked:
    self->enterUnlocked();
    break;
  default:
    break;
  }
}

void CarAlarm::cbBtn2Click(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::Unlocked || self->m_state == State::Locked)
    self->onSpecialAction();
}

void CarAlarm::cbBtn3DoubleClick(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state != State::Unlocked && self->m_state != State::Locked)
    return;
  self->m_cargoRelay->pulse(Config::CARGO_RELAY_PULSE_MS);
  self->m_led->playFlash(Config::FLASH_CARGO, Config::FLASH_CARGO_COUNT, true,
                         false, true, false, cbRestoreLed, self);
  self->m_buzzer->playPattern(Config::BEEP_CARGO, Config::BEEP_CARGO_COUNT);
}

// ── Motion started
// ──────────────────────────────────────────────────────────── Locked   → start
// pre-alarm countdown. PreAlarm → motion stopped then restarted: escalate to
// FullAlarm immediately.

void CarAlarm::cbMotion(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  switch (self->m_state) {
  case State::Locked:
    self->enterPreAlarm();
    break;
  case State::PreAlarm:
    self->m_motionActive = true;
    if (self->m_motionStoppedInPreAlarm) {
      // Disappeared then reappeared within the 5 s window → alarm now.
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
// ──────────────────────────────────────────────────────────── PreAlarm →
// record that motion has gone.  Do NOT touch the timer — the 5 s blink always
// runs to completion.  cbPreAlarmExpired will check m_motionActive and return
// to Locked if motion is still absent when the countdown finishes.

void CarAlarm::cbMotionEnd(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::PreAlarm) {
    self->m_motionActive = false;
    self->m_motionStoppedInPreAlarm = true;
  }
}

// ── Pre-alarm timer expired
// ─────────────────────────────────────────────────── Motion still present →
// FullAlarm. Motion gone          → quietly return to Locked (alarm never
// triggered).

void CarAlarm::cbPreAlarmExpired(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  self->m_preAlarmUpdate->stop();
  if (self->m_motionActive)
    self->enterFullAlarm();
  else
    self->enterLocked(/*silent=*/true); // no beep/relay — alarm never fired
}

void CarAlarm::cbAnyBtnAlarmReset(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::FullAlarm)
    self->enterLocked(/*silent=*/true); // dismissing alarm: no re-lock signal
}

void CarAlarm::cbPreAlarmUpdate(void *ctx) {
  static_cast<CarAlarm *>(ctx)->updatePreAlarmBlink();
}

void CarAlarm::cbRestoreLed(void *ctx) {
  static_cast<CarAlarm *>(ctx)->restoreLed();
}
