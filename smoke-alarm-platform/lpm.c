#include "contiki-conf.h"
//#include "sys/energest.h"
#include "lpm.h"
#include "stm32cube_hal_init.h"
#include "st-lib.h"
#include "clock.h"
#include "etimer.h"

//extern clock_time(void);
//void clock_adjust(void);

static void Error_Handler(void);
extern RTC_HandleTypeDef RtcHandle;

/*****
	获取 ticks: ticks = clock_time(), 获取next expiration：tts(time to stop) = etimer_next_expiration_time();
	设置唤醒时间 Wakeup Time：Wakeup Time = tts * 1/128s;
	Set ticks as (ticks + tts);
	Set seconds as (seconds + t/CLOCK_SECOND);
	Set uwTicks as (uwTicks + tts);
	Enter into stop mode;
	resume system clock(HSI);
	
	Enable tim2;
	*/
static void init_stop_mode_with_rtc(uint32_t sleep)
{
	uint32_t wakeUpCounter = 0;
	
    /* Disable Wakeup Counter */
    HAL_RTCEx_DeactivateWakeUpTimer(&RtcHandle);
    /* ## Setting the Wake up time ############################################*/
    /*  RTC Wakeup Interrupt Generation:
      Wakeup Time Base = (RTC_WAKEUPCLOCK_RTCCLK_DIV /LSE)
      Wakeup Time = Wakeup Time Base * WakeUpCounter 
      = (RTC_WAKEUPCLOCK_RTCCLK_DIV /LSE) * WakeUpCounter
        ==> WakeUpCounter = Wakeup Time / Wakeup Time Base
    
      To configure the wake up timer to 1/128 s the WakeUpCounter is set to 16:
      RTC_WAKEUPCLOCK_RTCCLK_DIV = RTCCLK_Div16 = 16 
      Wakeup Time Base = 16 /32768 Hz
      Wakeup Time = sleep * 1/ CLOCK_SECOND s
        ==> WakeUpCounter = Wakeup Time / (16 / 32768) = 16 */
	wakeUpCounter = sleep * 32768 / CLOCK_SECOND / 16;
    HAL_RTCEx_SetWakeUpTimer_IT(&RtcHandle, wakeUpCounter, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

static void init_low_power_sleep_mode(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    HAL_SuspendTick();

    /* Enable Power Control clock */
    __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is 
        clocked below the maximum system frequency, to update the voltage scaling value 
        regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2); /* For STM32L1: Low power sleep mode can only be entered when VCORE is in range 2 */

    /* Enable MSI Oscillator */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_OFF;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_1; /* Set temporary MSI range */
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }
    
    /* Select MSI as system clock source and configure the HCLK, PCLK1 and PCLK2 
        clocks dividers */
    RCC_ClkInitStruct.ClockType       = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider   = RCC_SYSCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider  = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider  = RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }
    /* Note: For STM32L1, to enable low power sleep mode, the system frequency  */
    /* should not exceed f_MSI range1.                                          */
    /* Set MSI range to 0 */
    __HAL_RCC_MSI_RANGE_CONFIG(RCC_MSIRANGE_0);

    /* Enable Ultra low power mode */
    HAL_PWREx_EnableUltraLowPower();
  
    /* Enable the fast wake up from Ultra low power mode */
    HAL_PWREx_EnableFastWakeUp();

    /* Disable Wakeup Counter */
    HAL_RTCEx_DeactivateWakeUpTimer(&RtcHandle);
    /* ## Setting the Wake up time ############################################*/
    /*  RTC Wakeup Interrupt Generation:
      Wakeup Time Base = (RTC_WAKEUPCLOCK_RTCCLK_DIV /LSE)
      Wakeup Time = Wakeup Time Base * WakeUpCounter 
      = (RTC_WAKEUPCLOCK_RTCCLK_DIV /LSE) * WakeUpCounter
        ==> WakeUpCounter = Wakeup Time / Wakeup Time Base
    
      To configure the wake up timer to 1/128 s the WakeUpCounter is set to 16:
      RTC_WAKEUPCLOCK_RTCCLK_DIV = RTCCLK_Div16 = 16 
      Wakeup Time Base = 16 /32768 Hz
      Wakeup Time = 1/128 s
        ==> WakeUpCounter = 1/128 / (16 / 32768) = 16 */
    HAL_RTCEx_SetWakeUpTimer_IT(&RtcHandle, 16, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
    // ~4s
    //HAL_RTCEx_SetWakeUpTimer_IT(&RtcHandle, 0x2616, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

static void enter_stop_mode_with_rtc(void)
{
    /* Enter Stop Mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}

static void enter_low_power_sleep_mode(void)
{
//    ticks = clock_time();
    /* Enter Sleep Mode */
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

/*****
	获取 ticks: ticks = clock_time(), 获取next expiration：tts(time to stop) = etimer_next_expiration_time();
	设置唤醒时间 Wakeup Time：Wakeup Time = tts * 1/128s;
	Set ticks as (ticks + tts);
	Set seconds as (seconds + t/CLOCK_SECOND);
	Set uwTicks as (uwTicks + tts);
	Enter into stop mode;
	resume system clock(HSI);
	
	Enable tim2;
	*/
static void stop_mode_with_rtc_exit()
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	
    /* Enable Power Control clock */
    __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is 
        clocked below the maximum system frequency, to update the voltage scaling value 
        regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Poll VOSF bit of in PWR_CSR. Wait until it is reset to 0 */
    while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOS) != RESET) {};

    /* Get the Oscillators configuration according to the internal RCC registers */
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

    /* After wake-up from STOP reconfigure the system clock: Enable HSI and PLL */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState            = RCC_HSE_OFF;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PLLDIV          = RCC_PLL_DIV3;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
        clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void low_power_sleep_mode_exit(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  
    /* Enable Power Control clock */
    __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is 
        clocked below the maximum system frequency, to update the voltage scaling value 
        regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Poll VOSF bit of in PWR_CSR. Wait until it is reset to 0 */
    while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOS) != RESET) {};

    /* Get the Oscillators configuration according to the internal RCC registers */
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

    /* After wake-up from STOP reconfigure the system clock: Enable HSI and PLL */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState            = RCC_HSE_OFF;
    RCC_OscInitStruct.MSIState            = RCC_MSI_OFF;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PLLDIV          = RCC_PLL_DIV3;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
        clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
  
//    ticks += 1;
    /* Resume Tick interrupt if disabled prior to sleep mode entry */
    HAL_ResumeTick();
    /* To Do*/
    /* set ticks to (ticks +1) */
}

clock_time_t lpm_suspend(void)
{
	HAL_SuspendTick();
	return etimer_next_expiration_time();
}

static void lpm_resume(clock_time_t sleep)
{
    unsigned long seconds;
    clock_time_t ticks;

    ticks = clock_time() + sleep;
    clock_set_ticks(ticks);
    seconds = clock_seconds() + sleep / CLOCK_SECOND;
    clock_set_seconds(seconds);
    HAL_ResumeTick();
    if(etimer_pending())
    {
        etimer_request_poll();
    }
}

void lpm_enter(LPM_MODE lpm_mode)
{
	clock_time_t sleep;
	sleep = lpm_suspend();
	if(sleep)
	{
		switch(lpm_mode)
		{
			case STOP_MODE_WITH_RTC:
				init_stop_mode_with_rtc((uint32_t)sleep);
				st_lib_bsp_led_on(LED2);
				enter_stop_mode_with_rtc();
				st_lib_bsp_led_off(LED2);
				stop_mode_with_rtc_exit();
				lpm_resume(sleep);
			break;
			// case LOW_POWER_SLEEP_MODE:
				// st_lib_bsp_led_on(LED2);
				// init_low_power_sleep_mode();
				// enter_low_power_sleep_mode();
				// st_lib_bsp_led_off(LED2);
				// low_power_sleep_mode_exit();
				// break;
			default: break;
		}
	}
}

/**
  * @brief  This function handles RTC Auto wake-up interrupt request.
  * @param  None
  * @retval None
  */
void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&RtcHandle);
}

static void Error_Handler(void)
{
  while(1)
  {
  }
}

/**
  * @brief  RTC Wake Up callback
  * @param  None
  * @retval None
  */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Clear Wake Up Flag */
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}
