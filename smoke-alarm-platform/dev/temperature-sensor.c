/*---------------------------------------------------------------------------*/
/**
 * \addtogroup fire-alarm-temperature-sensor
 * @{
 *
 * \file
 * Driver for the fire-alarm Temperature sensor (on expansion board)
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#include "lib/sensors.h"
#include "temperature-sensor.h"
#include "st-lib.h"
#include <math.h>

#define beta 3470.0f
#define r25c 10.0f  //25摄氏度时ntc的电阻值（千欧）
#define rRef 470.0f //分压电阻（千欧）
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
    volatile uint16_t sample[4];
    uint16_t sum = 0;
    int temperature;
    uint16_t average;
    float temperature_value, rth;
    uint8_t i;
    fire_alarm_board_get_temperature(sample);
    for(i = 0; i<4; i++) {
        sum += sample[i];
    }
    average = sum / 4;
    
    rth=rRef*(4096.0/average-1.0);
    temperature_value = (298.0f*beta)/logf(r25c/rth)/(beta/logf(r25c/rth)-298.0)-273.0;
    temperature = temperature_value * 10;
    return temperature;
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
SENSORS_SENSOR(temperature_sensor, TEMPERATURE_SENSOR,
               value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
