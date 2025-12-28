/*
 * main.c
 *
 *  Created on: 02-Jun-2018
 *      Author: kiran
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "stm32l4xx_hal.h"
#include "main_app.h"

/* Private function prototypes -----------------------------------------------*/
void GPIO_Init(void);
void Error_handler(void);
void UART2_Init(void);
void SystemClock_Config_HSE(uint8_t clock_freq);
void RTC_Init(void);
void RTC_CalendarConfig(void);

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
RTC_HandleTypeDef hrtc;

/**
  * @brief  Print a string to console over UART.
  * @param  format: Format string as in printf.
  * @param  ...: Additional arguments providing the data to print.
  * @retval None
  */
void printmsg(char *format,...)
{
  char str[80];

  /*Extract the the argument list using VA apis */
  va_list args;
  va_start(args, format);
  vsprintf(str, format,args);
  HAL_UART_Transmit(&huart2,(uint8_t *)str, strlen(str),HAL_MAX_DELAY);
  va_end(args);
}

int main(void)
{
  HAL_Init();
  GPIO_Init();
  SystemClock_Config_HSE(SYS_CLOCK_FREQ_50_MHZ);
  UART2_Init();
  RTC_Init();

  printmsg("This is RTC calendar Test program\r\n");

  if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
  {
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    printmsg("Woke up from STANDBY\r\n");
    HAL_GPIO_EXTI_Callback(0);
  }

  // //RTC_CalendarConfig();
  // //Enable the wakeup pin 1 in pwr_csr register
  // // HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1); //Pin A0

  // printmsg("Went to STANDBY mode\r\n");
  // HAL_PWR_EnterSTANDBYMode();

  // while(1);


    // RTC_CalendarConfig(); // Odkomentuj tylko raz, żeby ustawić datę, potem zakomentuj

    // 1. Włącz zegar kontrolera zasilania (PWR)
    __HAL_RCC_PWR_CLK_ENABLE();

    // 2. Wyczyść flagę wybudzenia, żeby nie obudził się od razu
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    // 3. Skonfiguruj rezystor Pull-Up dla PC13, który działa w Standby
    // (Dzięki temu przycisk będzie działał poprawnie nawet gdy procesor śpi)
    HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_C, PWR_GPIO_BIT_13);
    HAL_PWREx_EnablePullUpPullDownConfig();

    // 4. Włącz wybudzanie z pinu PC13 (WKUP2) na stan niski (LOW)
    // (Bo przycisk na Nucleo zwiera do masy)
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_LOW);

    printmsg("Went to STANDBY mode\r\n");

    // 5. Idź spać
    HAL_PWR_EnterSTANDBYMode();
    
    while(1);


  return 0;
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config_HSE(uint8_t clock_freq)
{
  RCC_OscInitTypeDef osc_init;
  RCC_ClkInitTypeDef clk_init;
  uint32_t FLatency =0;

  osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	osc_init.HSIState = RCC_HSI_ON;
	osc_init.HSICalibrationValue = 16;
	osc_init.PLL.PLLState = RCC_PLL_ON;
	osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSI;

  switch(clock_freq) {
        case SYS_CLOCK_FREQ_50_MHZ:{
            osc_init.PLL.PLLM = 4;
            osc_init.PLL.PLLN = 25;
            osc_init.PLL.PLLR= 2;
            osc_init.PLL.PLLP = 2;
            osc_init.PLL.PLLQ  = 2;

            clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | \
                                RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
            clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
            clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
            clk_init.APB1CLKDivider = RCC_HCLK_DIV2;
            clk_init.APB2CLKDivider = RCC_HCLK_DIV2;

            FLatency = FLASH_ACR_LATENCY_1WS;
            break;
    }
    case SYS_CLOCK_FREQ_80_MHZ: {
        osc_init.PLL.PLLM = 2;
        osc_init.PLL.PLLN = 20;
        osc_init.PLL.PLLR=2;
        osc_init.PLL.PLLP = 2;
        osc_init.PLL.PLLQ  = 2;

        clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | \
                            RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
        clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
        clk_init.APB1CLKDivider = RCC_HCLK_DIV2;
        clk_init.APB2CLKDivider = RCC_HCLK_DIV2;

        FLatency = FLASH_ACR_LATENCY_2WS;

        break;
    }

	default:
	  return;
	}

	if(HAL_RCC_OscConfig(&osc_init) != HAL_OK)
	{
		Error_handler();
	}


	if(HAL_RCC_ClockConfig(&clk_init,FLatency) != HAL_OK)
	{
		Error_handler();
	}

	//Systick configuration

  /*Configure the systick timer interrupt frequency (for every 1 ms) */
  uint32_t hclk_freq = HAL_RCC_GetHCLKFreq();
  HAL_SYSTICK_Config(hclk_freq/1000);

  /**Configure the Systick
  */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
void RTC_Init(void)
{
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat =RTC_HOURFORMAT_12;
  hrtc.Init.AsynchPrediv = 0x7F;
  hrtc.Init.SynchPrediv = 0xFF;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_LOW;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

  if( HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_handler();
  }
}

/**
  * @brief RTC calender configuration
  * @param None
  * @retval None
  */
void RTC_CalendarConfig(void)
{
  RTC_TimeTypeDef RTC_TimeInit;
  RTC_DateTypeDef RTC_DateInit;
  //this function does RTC Calendar Config
  //Lets configure the calendar for Time : 21:16:10 PM Date : 28 december 2025 SUNDAY

  RTC_TimeInit.Hours = 21;
  RTC_TimeInit.Minutes = 16;
  RTC_TimeInit.Seconds = 10;
  RTC_TimeInit.TimeFormat = RTC_HOURFORMAT12_PM;
  HAL_RTC_SetTime(&hrtc, &RTC_TimeInit,RTC_FORMAT_BIN);

  RTC_DateInit.Date = 28;
  RTC_DateInit.Month = RTC_MONTH_DECEMBER;
  RTC_DateInit.Year = 25;
  RTC_DateInit.WeekDay = RTC_WEEKDAY_SUNDAY;

  HAL_RTC_SetDate(&hrtc,&RTC_DateInit,RTC_FORMAT_BIN);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();


  GPIO_InitTypeDef ledgpio , buttongpio;

  ledgpio.Pin = GPIO_PIN_5;
  ledgpio.Mode = GPIO_MODE_OUTPUT_PP;
  ledgpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA,&ledgpio);

  buttongpio.Pin = GPIO_PIN_13;
  buttongpio.Mode = GPIO_MODE_IT_FALLING;
  buttongpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC,&buttongpio);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn,15,0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
void UART2_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate =115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.Mode = UART_MODE_TX;

  if ( HAL_UART_Init(&huart2) != HAL_OK )
  {
    //There is a problem
    Error_handler();
  }
}

/**
  * @brief  Returns the name of the day of the week.
  *         This function takes a number (1-7) and returns the corresponding
  *         day of the week as a string.
  * @param  number: The day of the week as a number (1 for Monday, 2 for Tuesday, etc.).
  * @retval char*: The name of the corresponding day of the week.
  */
char* getDayofweek(uint8_t number)
{
  char *weekday[] = { "Monday", "TuesDay", "Wednesday","Thursday","Friday","Saturday","Sunday"};

  return weekday[number-1];
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  RTC_TimeTypeDef RTC_TimeRead;
  RTC_DateTypeDef RTC_DateRead;

  HAL_RTC_GetTime(&hrtc,&RTC_TimeRead,RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc,&RTC_DateRead,RTC_FORMAT_BIN);

  printmsg("Current Time is : %02d:%02d:%02d\r\n",RTC_TimeRead.Hours,\
     RTC_TimeRead.Minutes,RTC_TimeRead.Seconds);
  printmsg("Current Date is : %02d-%2d-%2d  <%s> \r\n",RTC_DateRead.Month,RTC_DateRead.Date,\
     RTC_DateRead.Year,getDayofweek(RTC_DateRead.WeekDay));
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_handler(void)
{
  while(1);
}

