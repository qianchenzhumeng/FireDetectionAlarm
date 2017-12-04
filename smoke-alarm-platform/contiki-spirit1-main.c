/*---------------------------------------------------------------------------*/
/**
 * \addtogroup stm32nucleo-spirit1
 * @{
 *
 * \file
 * main file for stm32nucleo-spirit1 platform
 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "stm32cube_hal_init.h"
#include "contiki.h"
#include "contiki-net.h"
#include "sys/autostart.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/mac/frame802154.h"
#include "net/rime/rime.h"
#include "stm32l1xx.h"
#include "SPIRIT_Config.h"
#include "SPIRIT_Management.h"
#include "spirit1.h"
#include "spirit1-arch.h"
#include "hw-config.h"
#include "stdbool.h"
#include "dev/button-sensor.h"
#include "dev/radio-sensor.h"
#include "dev/temperature-sensor.h"
#include "dev/smoke-sensor.h"
#include "dev/buzzer-sensor.h"
//#include "lpm.h"

/*---------------------------------------------------------------------------*/

extern const struct sensors_sensor temperature_sensor;
extern const struct sensors_sensor smoke_sensor;
extern const struct sensors_sensor buzzer_sensor;

SENSORS(&button_sensor,
        &radio_sensor,
        &temperature_sensor,
        &smoke_sensor,
        &buzzer_sensor);
/*---------------------------------------------------------------------------*/
extern unsigned char node_mac[8];
/*---------------------------------------------------------------------------*/
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE * f)
#endif /* __GNUC__ */
/*---------------------------------------------------------------------------*/
PROCINIT(&etimer_process);
/*---------------------------------------------------------------------------*/
#define BUSYWAIT_UNTIL(cond, max_time) \
  do { \
    rtimer_clock_t t0; \
    t0 = RTIMER_NOW(); \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time))) ; \
  } while(0)
/*---------------------------------------------------------------------------*/
void stm32cube_hal_init();

/*---------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  stm32cube_hal_init();
  
  /* init LEDs */
  leds_init();

  /* Initialize Contiki and our processes. */
  clock_init();
  ctimer_init();
  rtimer_init();
  watchdog_init();
  process_init();
  process_start(&etimer_process, NULL);

  netstack_init();
  spirit_radio_driver.on();

  energest_init();

  process_start(&sensors_process, NULL);

  autostart_start(autostart_processes);

  watchdog_start();

  while(1) {
    int r = 0;
    do {
      r = process_run();
    } while(r > 0);
    //lpm_enter(STOP_MODE_WITH_RTC);
    /* Enter Sleep Mode*/
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
