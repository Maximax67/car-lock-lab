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
  tm.init(TIM2, Config::TIM_CLK_HZ, 0);

  g_btn1.init(GPIOA, 0, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);
  g_btn2.init(GPIOA, 1, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);
  g_btn3.init(GPIOA, 2, tm.allocate(), tm.allocate(),
              Config::BUTTON_DEBOUNCE_MS, Config::BUTTON_DOUBLE_CLICK_MS);

  g_motion.init(GPIOA, 7, tm.allocate(), Config::MOTION_DEBOUNCE_MS,
                ExtiPin::Trigger::Both);

  g_lockRelay.init(GPIOB, 0, tm.allocate());
  g_cargoRelay.init(GPIOB, 1, tm.allocate());

  g_buzzer.init(GPIOB, 4, tm.allocate());

  g_led.init(GPIOB, 5, // R
             GPIOB, 6, // G
             GPIOB, 7, // B
             tm.allocate());

  g_carAlarm.init(&g_btn1, &g_btn2, &g_btn3, &g_motion, &g_lockRelay,
                  &g_cargoRelay, &g_buzzer, &g_led, &tm);

  while (true) {
    __WFI();
  }
}
