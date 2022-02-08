/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    timer_if.c
  * @author  MCD Application Team
  * @brief   Configure RTC Alarm, Tick and Calendar manager
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include "timer_if.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "rtc.h"
#include "stm32wlxx_ll_rtc.h"

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/**
  * @brief Timer driver callbacks handler
  */
const UTIL_TIMER_Driver_s UTIL_TimerDriver =
{
  TIMER_IF_Init,
  NULL,

  TIMER_IF_StartTimer,
  TIMER_IF_StopTimer,

  TIMER_IF_SetTimerContext,
  TIMER_IF_GetTimerContext,

  TIMER_IF_GetTimerElapsedTime,
  TIMER_IF_GetTimerValue,
  TIMER_IF_GetMinimumTimeout,

  TIMER_IF_Convert_ms2Tick,
  TIMER_IF_Convert_Tick2ms,
};

/**
  * @brief SysTime driver callbacks handler
  */
const UTIL_SYSTIM_Driver_s UTIL_SYSTIMDriver =
{
  TIMER_IF_BkUp_Write_Seconds,
  TIMER_IF_BkUp_Read_Seconds,
  TIMER_IF_BkUp_Write_SubSeconds,
  TIMER_IF_BkUp_Read_SubSeconds,
  TIMER_IF_GetTime,
};

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RTC_PREDIV_TICK     1024            // RTC 分频 1024tick = 1s
#define MIN_ALARM_DELAY     3               // 警报的最小超时延迟
#define RTC_BKP_SECONDS     RTC_BKP_DR0     // 秒 备份寄存器
#define RTC_BKP_SUBSECONDS  RTC_BKP_DR1     // 亚秒 备份寄存器
#define RTC_BKP_MSBTICKS    RTC_BKP_DR2     // RTC溢出次数 备份寄存器

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static bool RTC_Initialized = false;        // RTC 初始化标志
static uint32_t RtcTimerContext = 0;        // RTC 时间上下文

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

static inline uint32_t GetTimerTicks(void)
{
  return (UINT32_MAX - LL_RTC_TIME_GetSubSecond(RTC));
}

static void TIMER_IF_BkUp_Write_MSBticks(uint32_t MSBticks)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_MSBTICKS, MSBticks);
}

static uint32_t TIMER_IF_BkUp_Read_MSBticks(void)
{
  uint32_t MSBticks;
  MSBticks = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_MSBTICKS);
  return MSBticks;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  UTIL_TIMER_IRQ_Handler();
}

void HAL_RTCEx_SSRUEventCallback(RTC_HandleTypeDef *hrtc)
{
  /*called every 48 days with 1024 ticks per seconds*/
  printf(">>Handler SSRUnderflow at %lu\n\r", GetTimerTicks());
  /*Increment MSBticks*/
  uint32_t MSB_ticks = TIMER_IF_BkUp_Read_MSBticks();
  TIMER_IF_BkUp_Write_MSBticks(MSB_ticks + 1);
}

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
UTIL_TIMER_Status_t TIMER_IF_Init(void)
{
  UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
  /* USER CODE BEGIN TIMER_IF_Init */

  if (RTC_Initialized == false)
  {
    hrtc.IsEnabled.RtcFeatures = UINT32_MAX;
    /*Init RTC*/
    MX_RTC_Init();
    /*Stop Timer */
    TIMER_IF_StopTimer();
    /** DeActivate the Alarm A enabled by MX during MX_RTC_Init() */
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    /*overload RTC feature enable*/
    hrtc.IsEnabled.RtcFeatures = UINT32_MAX;

    /*Enable Direct Read of the calendar registers (not through Shadow) */
    HAL_RTCEx_EnableBypassShadow(&hrtc);
    /*Initialise MSB ticks*/
    TIMER_IF_BkUp_Write_MSBticks(0);

    TIMER_IF_SetTimerContext();

    RTC_Initialized = true;
  }

  /* USER CODE END TIMER_IF_Init */
  return ret;
}

UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout)
{
  UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
  /* USER CODE BEGIN TIMER_IF_StartTimer */

  if (RTC_Initialized == true)
  {
    RTC_AlarmTypeDef sAlarm = {0};
    /*Stop timer if one is already started*/
    TIMER_IF_StopTimer();
    timeout += RtcTimerContext;

    // printf("Start timer: time=%lu, alarm=%lu\n\r",  GetTimerTicks(), timeout);
    /* starts timer*/
    sAlarm.BinaryAutoClr = RTC_ALARMSUBSECONDBIN_AUTOCLR_NO;
    sAlarm.AlarmTime.SubSeconds = UINT32_MAX - timeout;
    sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDBINMASK_NONE;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
    {
        Error_Handler();
    }
  }

  /* USER CODE END TIMER_IF_StartTimer */
  return ret;
}

UTIL_TIMER_Status_t TIMER_IF_StopTimer(void)
{
  UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
  /* USER CODE BEGIN TIMER_IF_StopTimer */

  /* Clear RTC Alarm Flag */
  __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
  /* Disable the Alarm A interrupt */
  HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
  /*overload RTC feature enable*/
  hrtc.IsEnabled.RtcFeatures = UINT32_MAX;

  /* USER CODE END TIMER_IF_StopTimer */
  return ret;
}

uint32_t TIMER_IF_SetTimerContext(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_SetTimerContext */

  /*store time context*/
  RtcTimerContext = GetTimerTicks();
  ret = RtcTimerContext;
//   printf("TIMER_IF_SetTimerContext=%lu\n\r", ret);

  /* USER CODE END TIMER_IF_SetTimerContext */
  return ret;
}

uint32_t TIMER_IF_GetTimerContext(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTimerContext */

  ret = RtcTimerContext;
//   printf("TIMER_IF_GetTimerContext=%lu\n\r", ret);

  /* USER CODE END TIMER_IF_GetTimerContext */
  return ret;
}

uint32_t TIMER_IF_GetTimerElapsedTime(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTimerElapsedTime */

  ret = GetTimerTicks() - RtcTimerContext;
//   printf("TIMER_IF_GetTimerElapsedTime=%lu\n\r", ret);

  /* USER CODE END TIMER_IF_GetTimerElapsedTime */
  return ret;
}

uint32_t TIMER_IF_GetTimerValue(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTimerValue */

  if (RTC_Initialized == true)
    ret = GetTimerTicks();

  /* USER CODE END TIMER_IF_GetTimerValue */
  return ret;
}

uint32_t TIMER_IF_GetMinimumTimeout(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetMinimumTimeout */

  ret = MIN_ALARM_DELAY;

  /* USER CODE END TIMER_IF_GetMinimumTimeout */
  return ret;
}

uint32_t TIMER_IF_Convert_ms2Tick(uint32_t timeMilliSec)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_Convert_ms2Tick */

  ret = (uint32_t)((uint64_t)timeMilliSec * RTC_PREDIV_TICK / 1000);

  /* USER CODE END TIMER_IF_Convert_ms2Tick */
  return ret;
}

uint32_t TIMER_IF_Convert_Tick2ms(uint32_t tick)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_Convert_Tick2ms */

  ret = (uint32_t)((uint64_t)tick * 1000 / RTC_PREDIV_TICK);

  /* USER CODE END TIMER_IF_Convert_Tick2ms */
    return ret;
}

void TIMER_IF_DelayMs(uint32_t delay)
{
  /* USER CODE BEGIN TIMER_IF_DelayMs */

  uint32_t delayTicks = TIMER_IF_Convert_ms2Tick(delay);
  uint32_t timeout = GetTimerTicks();

  /* Wait delay ms */
  while (((GetTimerTicks() - timeout)) < delayTicks)
  {
    __NOP();
  }

  /* USER CODE END TIMER_IF_DelayMs */
}

uint32_t TIMER_IF_GetTime(uint16_t *mSeconds)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTime */

  uint64_t ticks = 0;
  uint32_t timerValueLsb = GetTimerTicks();
  uint32_t timerValueMSB = TIMER_IF_BkUp_Read_MSBticks();

  ticks = (((uint64_t) timerValueMSB) << 32) + timerValueLsb;

  ret = (uint32_t)(ticks / RTC_PREDIV_TICK);
  if (mSeconds != NULL)
    *mSeconds = TIMER_IF_Convert_Tick2ms(ticks % RTC_PREDIV_TICK);

  /* USER CODE END TIMER_IF_GetTime */
    return ret;
}

void TIMER_IF_BkUp_Write_Seconds(uint32_t Seconds)
{
  /* USER CODE BEGIN TIMER_IF_BkUp_Write_Seconds */

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_SECONDS, Seconds);

  /* USER CODE END TIMER_IF_BkUp_Write_Seconds */
}

void TIMER_IF_BkUp_Write_SubSeconds(uint32_t SubSeconds)
{
  /* USER CODE BEGIN TIMER_IF_BkUp_Write_SubSeconds */

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_SUBSECONDS, SubSeconds);

  /* USER CODE END TIMER_IF_BkUp_Write_SubSeconds */
}

uint32_t TIMER_IF_BkUp_Read_Seconds(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_BkUp_Read_Seconds */

  ret = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_SECONDS);

  /* USER CODE END TIMER_IF_BkUp_Read_Seconds */
    return ret;
}

uint32_t TIMER_IF_BkUp_Read_SubSeconds(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_BkUp_Read_SubSeconds */

  ret = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_SUBSECONDS);

  /* USER CODE END TIMER_IF_BkUp_Read_SubSeconds */
    return ret;
}

/* USER CODE BEGIN EF */
/**
  * @brief Provide a tick value in millisecond measured using RTC
  * @note This function overwrites the __weak one from HAL
  * @retval tick value
  */
uint32_t HAL_GetTick(void)
{
  if (RTC_Initialized == true)
  {
    /* TIMER_IF can be based onother counter the SysTick e.g. RTC */
    return TIMER_IF_GetTimerValue();
  }
  else
  {
    return uwTick;
  }
}

/**
  * @brief This function provides delay (in ms)
  * @param Delay: specifies the delay time length, in milliseconds.
  * @retval None
  */
void HAL_Delay(__IO uint32_t Delay)
{
  if (RTC_Initialized == true)
  {
    /* TIMER_IF can be based onother counter the SysTick e.g. RTC */
    TIMER_IF_DelayMs(Delay);
  }
  else
  {
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    if (wait < HAL_MAX_DELAY)
        wait += (uint32_t)(uwTickFreq);

    while ((HAL_GetTick() - tickstart) < wait);
  }
}
/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
