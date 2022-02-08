/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for LEDTask */
osThreadId_t LEDTaskHandle;
const osThreadAttr_t LEDTask_attributes = {
  .name = "LEDTask",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};
/* Definitions for KEYTack */
osThreadId_t KEYTackHandle;
const osThreadAttr_t KEYTack_attributes = {
  .name = "KEYTack",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};
/* Definitions for myQueue01 */
osMessageQueueId_t myQueue01Handle;
const osMessageQueueAttr_t myQueue01_attributes = {
  .name = "myQueue01"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Task_LED(void *argument);
void Tack_key(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of myQueue01 */
  myQueue01Handle = osMessageQueueNew (2, sizeof(uint8_t), &myQueue01_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of LEDTask */
  LEDTaskHandle = osThreadNew(Task_LED, NULL, &LEDTask_attributes);

  /* creation of KEYTack */
  KEYTackHandle = osThreadNew(Tack_key, NULL, &KEYTack_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Task_LED */
/**
* @brief Function implementing the LEDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_LED */
void Task_LED(void *argument)
{
  /* USER CODE BEGIN Task_LED */
  /* Infinite loop */
  for(;;)
  {
	static uint8_t msg = 0;
	if (osMessageQueueGetCount(myQueue01Handle) != 0){
		osMessageQueueGet (myQueue01Handle, &msg, NULL, 0);
		if(msg == 0){
			LED_RED_OFF;
			LED_GREEN_OFF;
		}
		else if(msg == 1){
			LED_GREEN_ON;
		}
		else if(msg == 2){
			LED_RED_ON;
		}
	}
    uint8_t aTxBuffer[]="123";
    HAL_UART_Transmit(&huart2, aTxBuffer, sizeof(aTxBuffer),0xffff);
    osDelay(1);

  }
  /* USER CODE END Task_LED */
}

/* USER CODE BEGIN Header_Tack_key */
/**
* @brief Function implementing the KEYTack thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Tack_key */
void Tack_key(void *argument)
{
  /* USER CODE BEGIN Tack_key */
  /* Infinite loop */
  for(;;)
  {
	  if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == KEY2_PRESS){
		  osDelay(10);
		  if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == KEY2_PRESS){
	//		  LED_GREEN_TOGGLE;
			  uint32_t tickstart = HAL_GetTick();
			  uint8_t KEY_State = 0;
	//		  if (wait < HAL_MAX_DELAY)
	//		  {
	//		    wait += (uint32_t)(uwTickFreq);
	//		  }

			  while(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == KEY2_PRESS){
				  osDelay(100);
			  }
			  if((HAL_GetTick() - tickstart) < KEY2_TIME1){
				  KEY_State = 0;
			  }
			  else if((HAL_GetTick() - tickstart) < KEY2_TIME2){
				  KEY_State = 1;
			  }
			  else{
				  KEY_State = 2;
			  }
			  osMessageQueuePut(myQueue01Handle, &KEY_State, 0, 10);
		  }
	  }
	  osDelay(1);
  }
  /* USER CODE END Tack_key */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
