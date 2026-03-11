#include "App/car_alarm.hpp"
#include "App/config.hpp"
#include "Drivers/button.hpp"
#include "Drivers/buzzer.hpp"
#include "Drivers/motion_sensor.hpp"
#include "Drivers/relay.hpp"
#include "Drivers/rgb_led.hpp"
#include "HAL/Timer/timer_manager.hpp"
#include "stm32f103x6.h"

static Button g_btn1;
static Button g_btn2;
static Button g_btn3;
static MotionSensor g_motion;
static Relay g_lockRelay;
static Relay g_cargoRelay;
static Buzzer g_buzzer;
static RgbLed g_led;
static CarAlarm g_carAlarm;

int main() {
  auto &tm = TimerManager::instance();
  tm.init(Config::CORE_CLK_HZ, Config::SYSTICK_NVIC_PRIORITY);

  g_btn1.init(Config::BTN1_PORT, Config::BTN1_PIN, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);
  g_btn2.init(Config::BTN2_PORT, Config::BTN2_PIN, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);
  g_btn3.init(Config::BTN3_PORT, Config::BTN3_PIN, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);

  g_motion.init(Config::MOTION_PORT, Config::MOTION_PIN, tm.allocate(),
                Config::MOTION_DEBOUNCE_MS, Config::MOTION_TRIGGER);

  g_lockRelay.init(Config::LOCK_RELAY_PORT, Config::LOCK_RELAY_PIN,
                   tm.allocate());
  g_cargoRelay.init(Config::CARGO_RELAY_PORT, Config::CARGO_RELAY_PIN,
                    tm.allocate());

  g_buzzer.init(Config::BUZZER_PORT, Config::BUZZER_PIN, tm.allocate());

  g_led.init(Config::LED_R_PORT, Config::LED_R_PIN, Config::LED_G_PORT,
             Config::LED_G_PIN, Config::LED_B_PORT, Config::LED_B_PIN,
             Config::LED_BLINK_TIM, Config::LED_BLINK_TIM_CLK_HZ,
             Config::LED_BLINK_TIM_NVIC_PRIORITY, tm.allocate());

  g_carAlarm.init(&g_btn1, &g_btn2, &g_btn3, &g_motion, &g_lockRelay,
                  &g_cargoRelay, &g_buzzer, &g_led, &tm);

  while (true) {
    __WFI();
  }
}
