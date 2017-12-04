#ifndef FIRE_ALARM_BOARD_H_
#define FIRE_ALARM_BOARD_H_

#include <stdint.h>

/* ## Definition of ADC related resources ################################### */
/* Definition of ADCx clock resources */
#define ADCx                            ADC1
#define ADCx_CLK_ENABLE()               __ADC1_CLK_ENABLE()
#define ADCx_CLK_DISABLE()              __ADC1_CLK_DISABLE()

/* Definition of ADCx channels */
#define ADCx_CHANNEL_Smoke                   ADC_CHANNEL_0
#define ADCx_CHANNEL_Temp                   ADC_CHANNEL_1

/* Definition of ADCx channels pins */
#define ADCx_CHANNEL_GPIO_CLK_ENABLE() __GPIOA_CLK_ENABLE()
#define ADCx_CHANNEL_GPIO_PORT         GPIOA
#define ADCx_CHANNEL_PIN               GPIO_PIN_0 | GPIO_PIN_1

/* Definition of smoke board control pins */
#define FIRE_BOARD_SC_LED_CTRL_GPIO_CLK_ENABLE()    __GPIOC_CLK_ENABLE()
#define FIRE_BOARD_SC_LED_CTRL_GPIO_PORT            GPIOC
#define FIRE_BOARD_SC_LED_CTRL_PIN                  GPIO_PIN_0

#define FIRE_BOARD_SC_OA_CTRL_GPIO_CLK_ENABLE()     __GPIOB_CLK_ENABLE()
#define FIRE_BOARD_SC_OA_CTRL_GPIO_PORT             GPIOB
#define FIRE_BOARD_SC_OA_CTRL_PIN                   GPIO_PIN_0

#define TIMx                                        TIM3
#define TIMx_CLK_ENABLE()                           __TIM3_CLK_ENABLE()
#define TIMx_CLK_DISABLE()                          __TIM3_CLK_DISABLE()
#define TIMx_CHANNEL_GPIO_PORT()                      __GPIOB_CLK_ENABLE()  
#define TIMx_GPIO_PORT_CHANNEL                      GPIOB
#define TIMx_GPIO_AF_CHANNEL                        GPIO_AF2_TIM3
#define TIMx_GPIO_PIN_CHANNEL                       GPIO_PIN_5
#define TIMx_FREQUENCY_HZ                           ((uint32_t)1000)
#define TIMx_PRESCALER                              ((uint32_t)(F_CPU / TIMx_FREQUENCY_HZ -1))
#define TIMx_PERIOD_VALUE                           ((uint32_t)999)
#define TIMx_PULSE_VALUE                            ((uint32_t)700)

uint8_t FAB__isInitialized(void);
void FAB_Init(void);
void FAB_GetTemperature(uint16_t * pData);
void FAB_GetSmoke(uint16_t * pData);
void FAB_ActivateBuzzer(void);
void FAB_DeactivateBuzzer(void);

#endif /* FIRE_ALARM_BOARD_H_ */
