/*---------------------------------------------------------------------------*/
/**
 * \addtogroup fire-alarm-smoke-sensor
 * @{
 *
 * \file
 * Driver for the fire-alarm Smoke sensor (on expansion board)
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#include "lib/sensors.h"
#include "smoke-sensor.h"
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
    _active = 1;
}
/*---------------------------------------------------------------------------*/
static void
deactivate(void)
{
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
    volatile uint16_t sample[8];
    uint16_t dark, light;
    uint16_t sum_dark = 0, sum_light = 0;
    uint8_t i;
    fire_alarm_board_get_smoke(sample);
    for(i = 0; i < 4; i++) {
        sum_dark += sample[i];
        sum_light += sample[i+4];
    }
    dark = sum_dark / 4;
    light = sum_light / 4;
    if(light > dark) {
        return (int)(light-dark);
    }
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
SENSORS_SENSOR(smoke_sensor, SMOKE_SENSOR,
               value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
