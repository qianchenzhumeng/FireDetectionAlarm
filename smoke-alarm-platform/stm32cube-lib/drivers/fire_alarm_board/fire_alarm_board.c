#include "fire_alarm_board.h"
#include "st-lib.h"
#include "platform-conf.h"

static uint8_t BoardInitialized = 0;
static ADC_HandleTypeDef    AdcHandle;
static TIM_HandleTypeDef    TimHandle;
uint8_t FAB_isInitialized(void)
{
    return BoardInitialized;
}

void FAB_Init(void)
{
    GPIO_InitTypeDef    GPIO_InitStruct;
    TIM_OC_InitTypeDef  TIM_OCInitStructure;
    
    ADCx_CHANNEL_GPIO_CLK_ENABLE();
    ADCx_CLK_ENABLE();
    FIRE_BOARD_SC_LED_CTRL_GPIO_CLK_ENABLE();
    FIRE_BOARD_SC_OA_CTRL_GPIO_CLK_ENABLE();
    
    /* Configure GPIO pin of the selected ADC channel */
    GPIO_InitStruct.Pin = ADCx_CHANNEL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ADCx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);

    /* Configuration of AdcHandle init structure: ADC parameters and regular group */
    AdcHandle.Instance = ADCx;
    HAL_ADC_DeInit(&AdcHandle);

    AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV4;
    AdcHandle.Init.Resolution            = ADC_RESOLUTION12b;
    AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    AdcHandle.Init.ScanConvMode          = DISABLE;
    AdcHandle.Init.EOCSelection          = EOC_SINGLE_CONV;
    AdcHandle.Init.LowPowerAutoWait      = ADC_AUTOWAIT_DISABLE;
    AdcHandle.Init.LowPowerAutoPowerOff  = ADC_AUTOPOWEROFF_DISABLE;
    AdcHandle.Init.ChannelsBank          = ADC_CHANNELS_BANK_A;
    AdcHandle.Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 rank converted at each conversion trig, and because discontinuous mode is enabled */
    AdcHandle.Init.NbrOfConversion       = 1;
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;                        /* Sequencer of regular group will convert the sequence in several sub-divided sequences */
    AdcHandle.Init.NbrOfDiscConversion   = 1;
    AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Trig of conversion start done manually by software, without external event */
    AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    HAL_ADC_Init(&AdcHandle);
    
    /* Configure GPIO pin for SC LED control */
    GPIO_InitStruct.Pin = FIRE_BOARD_SC_LED_CTRL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(FIRE_BOARD_SC_LED_CTRL_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(FIRE_BOARD_SC_LED_CTRL_GPIO_PORT,
                      FIRE_BOARD_SC_LED_CTRL_PIN, GPIO_PIN_RESET);
                      
    /* Configure GPIO pin for SC LED control */
    GPIO_InitStruct.Pin = FIRE_BOARD_SC_OA_CTRL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(FIRE_BOARD_SC_OA_CTRL_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(FIRE_BOARD_SC_OA_CTRL_GPIO_PORT,
                      FIRE_BOARD_SC_OA_CTRL_PIN, GPIO_PIN_RESET);
    
    /* buzzer */
    TIMx_CLK_ENABLE();
    TimHandle.Instance = TIMx;
    TimHandle.Init.Prescaler         = TIMx_PRESCALER;
    TimHandle.Init.Period            = TIMx_PERIOD_VALUE;
    TimHandle.Init.ClockDivision     = 0;
    TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&TimHandle);
    
    TIM_OCInitStructure.OCMode       = TIM_OCMODE_PWM1;
    TIM_OCInitStructure.OCPolarity   = TIM_OCPOLARITY_HIGH;
    TIM_OCInitStructure.OCFastMode   = TIM_OCFAST_DISABLE;
    TIM_OCInitStructure.OCIdleState  = TIM_OCIDLESTATE_RESET;
    TIM_OCInitStructure.Pulse        = TIMx_PULSE_VALUE;
    HAL_TIM_PWM_ConfigChannel(&TimHandle, &TIM_OCInitStructure, TIM_CHANNEL_2);
    
    /* Enable GPIO Channels Clock */
    TIMx_CHANNEL_GPIO_PORT();
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = TIMx_GPIO_AF_CHANNEL;
    GPIO_InitStruct.Pin = TIMx_GPIO_PIN_CHANNEL;
    HAL_GPIO_Init(TIMx_GPIO_PORT_CHANNEL, &GPIO_InitStruct);
    //HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
}
void FAB_GetTemperature(uint16_t * pData)
{
    ADC_ChannelConfTypeDef   sConfig;
    
    sConfig.Channel      = ADCx_CHANNEL_Temp;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
    HAL_ADC_ConfigChannel(&AdcHandle, &sConfig);

    ADCx_CLK_ENABLE();
    
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[0] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[1] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[2] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[3] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Stop(&AdcHandle);
    
    ADCx_CLK_DISABLE();
}

void FAB_GetSmoke(uint16_t * pData)
{
    ADC_ChannelConfTypeDef   sConfig;
    
    sConfig.Channel      = ADCx_CHANNEL_Smoke;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
    HAL_ADC_ConfigChannel(&AdcHandle, &sConfig);
    
    ADCx_CLK_ENABLE();
    
    /* Sample Dark */
    HAL_GPIO_WritePin(FIRE_BOARD_SC_OA_CTRL_GPIO_PORT,
                      FIRE_BOARD_SC_OA_CTRL_PIN, GPIO_PIN_SET);
    clock_delay(20);
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[0] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[1] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[2] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[3] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    
    /* Sample Light */
    HAL_GPIO_WritePin(FIRE_BOARD_SC_LED_CTRL_GPIO_PORT,
                      FIRE_BOARD_SC_LED_CTRL_PIN, GPIO_PIN_SET);
    clock_delay(20);
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[4] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[5] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[6] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));
    HAL_ADC_Start(&AdcHandle);
    HAL_ADC_PollForConversion(&AdcHandle, (1000/TIMx_FREQUENCY_HZ));
    pData[7] = (uint16_t)(HAL_ADC_GetValue(&AdcHandle));

    HAL_GPIO_WritePin(FIRE_BOARD_SC_LED_CTRL_GPIO_PORT,
                      FIRE_BOARD_SC_LED_CTRL_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(FIRE_BOARD_SC_OA_CTRL_GPIO_PORT,
                      FIRE_BOARD_SC_OA_CTRL_PIN, GPIO_PIN_RESET);
    HAL_ADC_Stop(&AdcHandle);
    
    ADCx_CLK_DISABLE();
}

void FAB_ActivateBuzzer(void)
{
    TIMx_CLK_ENABLE();
    HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
}

void FAB_DeactivateBuzzer(void)
{
    HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
    TIMx_CLK_DISABLE();
}
