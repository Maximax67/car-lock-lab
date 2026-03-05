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

  // ── Button 1: lock / unlock (single-click) ────────────────────────────
  m_btn1->setOnSingleClick(cbBtn1Click, this);

  // ── Button 2: special action (single-click) ───────────────────────────
  m_btn2->setOnSingleClick(cbBtn2Click, this);

  // ── Button 3: cargo door (double-click) ───────────────────────────────
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);

  // ── Motion sensor ─────────────────────────────────────────────────────
  m_motion->setOnMotion(cbMotion, this);

  // FIX 1: Start in Locked state (alarm armed from power-on).
  enterLocked();
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

  // Solid green.
  m_led->setColor(/*r=*/false, /*g=*/true, /*b=*/false);

  // Short confirmation beep.
  m_buzzer->beep(Config::UNLOCK_BEEP_MS);

  // Pulse lock relay.
  m_lockRelay->pulse(Config::LOCK_RELAY_PULSE_MS);

  // Alarm-reset override: while unlocked the buttons no longer need to
  // reset the alarm, so re-wire btn1/btn2 to their normal roles.
  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);
}

void CarAlarm::enterLocked() {
  m_state = State::Locked;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();
  m_buzzer->stop();

  // Solid red.
  m_led->setColor(/*r=*/true, /*g=*/false, /*b=*/false);

  // Long confirmation beep.
  m_buzzer->beep(Config::LOCK_BEEP_MS);

  // Pulse lock relay.
  m_lockRelay->pulse(Config::LOCK_RELAY_PULSE_MS);

  // Arm motion sensor.
  m_motion->enable();

  // Restore normal button callbacks (in case we came from FullAlarm).
  m_btn1->setOnSingleClick(cbBtn1Click, this);
  m_btn2->setOnSingleClick(cbBtn2Click, this);
  m_btn3->setOnDoubleClick(cbBtn3DoubleClick, this);
}

void CarAlarm::enterPreAlarm() {
  m_state = State::PreAlarm;
  m_preAlarmElapsedMs = 0;
  m_motionSeenInPreAlarm = true; // first motion has already been seen

  m_buzzer->stop();

  // Start blinking at the slow initial rate (half-period = full/2).
  m_led->startBlink(Config::PRE_ALARM_BLINK_START_MS / 2,
                    /*r=*/true, /*g=*/false, /*b=*/false);

  // 5-second countdown to full alarm.
  m_preAlarmTimer->start(Config::PRE_ALARM_DURATION_MS,
                         /*oneShot=*/true, cbPreAlarmExpired, this);

  // 100 ms update ticker to interpolate blink speed.
  m_preAlarmUpdate->start(Config::PRE_ALARM_UPDATE_STEP_MS,
                          /*oneShot=*/false, cbPreAlarmUpdate, this);
}

void CarAlarm::enterFullAlarm() {
  m_state = State::FullAlarm;

  m_preAlarmTimer->stop();
  m_preAlarmUpdate->stop();

  // Fast red blink (~125 ms full period).
  m_led->startBlink(Config::ALARM_LED_HALF_PERIOD_MS,
                    /*r=*/true, /*g=*/false, /*b=*/false);

  // Continuous buzzer pattern (150 ms on / 150 ms off).
  m_buzzer->playPattern(Config::BEEP_ALARM, Config::BEEP_ALARM_COUNT,
                        /*repeat=*/true);

  // Any button click → reset alarm back to Locked.
  m_btn1->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn2->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn3->setOnSingleClick(cbAnyBtnAlarmReset, this);
  m_btn3->setOnDoubleClick(cbAnyBtnAlarmReset, this);
}

// ─────────────────────────────────────────────────────────────────────────────
// LED restore helper
// Called as a flash-completion callback so the LED returns to the solid
// colour that matches the current state after any one-shot flash.
// Only acts in Unlocked/Locked — blinking states manage their own LED.
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::restoreLed() {
  switch (m_state) {
  case State::Unlocked:
    m_led->setColor(/*r=*/false, /*g=*/true, /*b=*/false);
    break;
  case State::Locked:
    m_led->setColor(/*r=*/true, /*g=*/false, /*b=*/false);
    break;
  default:
    // PreAlarm and FullAlarm drive the LED themselves — do nothing.
    break;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Button 2 special action — default: 300 ms blue flash then restore LED.
// Override onSpecialAction() in a subclass to replace this behaviour.
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::onSpecialAction() {
  m_led->playFlash(Config::FLASH_BTN2, Config::FLASH_BTN2_COUNT,
                   /*r=*/false, /*g=*/false, /*b=*/true,
                   /*repeat=*/false, cbRestoreLed, this);
}

// ─────────────────────────────────────────────────────────────────────────────
// Pre-alarm interpolation (called every 100 ms from m_preAlarmUpdate)
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::updatePreAlarmBlink() {
  m_preAlarmElapsedMs += Config::PRE_ALARM_UPDATE_STEP_MS;

  // Linear interpolation: period goes from START → END over DURATION.
  uint32_t elapsed = m_preAlarmElapsedMs;
  uint32_t total = Config::PRE_ALARM_DURATION_MS;
  uint32_t startMs = Config::PRE_ALARM_BLINK_START_MS;
  uint32_t endMs = Config::PRE_ALARM_BLINK_END_MS;

  if (elapsed > total)
    elapsed = total;

  // Full period (integer, rounded).
  uint32_t period = startMs - (startMs - endMs) * elapsed / total;

  // setBlinkPeriod takes the half-period.
  m_led->setBlinkPeriod(period / 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static callback trampolines
// ─────────────────────────────────────────────────────────────────────────────

void CarAlarm::cbBtn1Click(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  switch (self->m_state) {
  case State::Unlocked:
    self->enterLocked();
    break;
  case State::Locked:
    self->enterUnlocked();
    break;
  default:
    break; // ignored in alarm states
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

  // Cargo door: relay pulse + LED flash (restores LED when done) + buzzer.
  self->m_cargoRelay->pulse(Config::CARGO_RELAY_PULSE_MS);
  self->m_led->playFlash(Config::FLASH_CARGO, Config::FLASH_CARGO_COUNT,
                         /*r=*/true, /*g=*/false, /*b=*/true,
                         /*repeat=*/false, cbRestoreLed, self);
  self->m_buzzer->playPattern(Config::BEEP_CARGO, Config::BEEP_CARGO_COUNT);
}

void CarAlarm::cbMotion(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  switch (self->m_state) {
  case State::Locked:
    self->enterPreAlarm();
    break;

  case State::PreAlarm:
    // Second motion trigger (motion disappeared then reappeared)
    // → escalate to full alarm immediately.
    if (self->m_motionSeenInPreAlarm) {
      self->m_preAlarmTimer->stop();
      self->m_preAlarmUpdate->stop();
      self->enterFullAlarm();
    }
    break;

  default:
    break; // ignore in Unlocked / FullAlarm
  }
}

void CarAlarm::cbAnyBtnAlarmReset(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  if (self->m_state == State::FullAlarm)
    self->enterLocked(); // returns to Locked (still armed)
}

void CarAlarm::cbPreAlarmExpired(void *ctx) {
  auto *self = static_cast<CarAlarm *>(ctx);
  self->m_preAlarmUpdate->stop();
  self->enterFullAlarm();
}

void CarAlarm::cbPreAlarmUpdate(void *ctx) {
  static_cast<CarAlarm *>(ctx)->updatePreAlarmBlink();
}

void CarAlarm::cbRestoreLed(void *ctx) {
  static_cast<CarAlarm *>(ctx)->restoreLed();
}
