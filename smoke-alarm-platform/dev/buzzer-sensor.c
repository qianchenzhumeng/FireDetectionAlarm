/*---------------------------------------------------------------------------*/
/**
 * \addtogroup buzzer-sensor
 * @{
 *
 * \file
 * Driver for the fire alarm board buzzer sensor (on expansion board)
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#include "lib/sensors.h"
#include "buzzer-sensor.h"
#include "st-lib.h"
/*---------------------------------------------------------------------------*/
static int _active = 0;
/*---------------------------------------------------------------------------*/
static void init(void)
{
    if(!fire_alarm_board_is_initialized()) {
        fire_alarm_board_init();
        _active = 1;
    }
}
/*---------------------------------------------------------------------------*/
static void activate(void)
{
    fire_alarm_board_activate_buzzer();
    _active = 1;
}
/*---------------------------------------------------------------------------*/
static void
deactivate(void)
{
    fire_alarm_board_deactivate_buzzer();
    _active = 0;
}
/*---------------------------------------------------------------------------*/
static int
active(void)
{
    return _active;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
    return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case SENSORS_HW_INIT:
    init();
    return 1;
  case SENSORS_ACTIVE:
    if(value) {
      activate();
    } else {
      deactivate();
    }
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
  case SENSORS_READY:
    return active();
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(buzzer_sensor, BUZZER_SENSOR,
               value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
