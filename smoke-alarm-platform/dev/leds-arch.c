/*---------------------------------------------------------------------------*/
/**
 * \addtogroup stm32nucleo-spirit1-peripherals
 * @{
 *
 * \file
 * Driver for the stm32nucleo-spirit1 LEDs
 */
/*---------------------------------------------------------------------------*/
#include "contiki-conf.h"
#include "dev/leds.h"
#include "st-lib.h"
/*---------------------------------------------------------------------------*/

extern st_lib_gpio_typedef *st_lib_a_led_gpio_port[];
extern const uint16_t st_lib_a_led_gpio_pin[];

extern st_lib_gpio_typedef *st_lib_gpio_port[];
extern const uint16_t st_lib_gpio_pin[];
/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
    /* The Red LED */
    st_lib_radio_shield_led_init(RADIO_SHIELD_LED);
    st_lib_radio_shield_led_off(RADIO_SHIELD_LED);
    
    /* The Yellow LED */
    st_lib_bsp_led_init(LED2);
    st_lib_bsp_led_off(LED2);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  unsigned char ret = 0;

  if(st_lib_hal_gpio_read_pin(st_lib_a_led_gpio_port[RADIO_SHIELD_LED],
                              st_lib_a_led_gpio_pin[RADIO_SHIELD_LED])) {
    ret |= LEDS_RED;
  }
  if(st_lib_hal_gpio_read_pin(st_lib_gpio_port[LED2], st_lib_gpio_pin[LED2])) {
    ret |= LEDS_YELLOW;
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{  
  if(leds & LEDS_YELLOW) {
    st_lib_bsp_led_on(LED2);
  } else {
    st_lib_bsp_led_off(LED2);
  }

  if(leds & LEDS_RED) {
    st_lib_radio_shield_led_on(RADIO_SHIELD_LED);
  } else {
    st_lib_radio_shield_led_off(RADIO_SHIELD_LED);
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
