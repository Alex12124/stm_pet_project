/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "adc.h"
#include "tim.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "pi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
volatile uint16_t adc_buffer[2];
volatile uint16_t disp_real_milli_voltage = 0;
volatile uint16_t disp_desired_milli_voltage = 0;

enum {
	MAX_THRESHOLD_CONTROL_VOLTAGE = 3300,
	MIN_THRESHOLD_CONTROL_VOLTAGE = 0,
};
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId OledTaskHandle;
/* USER CODE END Variables */
osThreadId control_taskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void OledTask(void const * argument);
/* USER CODE END FunctionPrototypes */

void ControlTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

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

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of control_task */
  osThreadDef(control_task, ControlTask, osPriorityNormal, 0, 128);
  control_taskHandle = osThreadCreate(osThread(control_task), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(oled_task, OledTask, osPriorityBelowNormal, 0, 128);
  OledTaskHandle = osThreadCreate(osThread(oled_task), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_ControlTask */
/**
  * @brief  Function implementing the control_task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_ControlTask */
void ControlTask(void const * argument)
{
  /* USER CODE BEGIN ControlTask */
  vTaskSetApplicationTaskTag(NULL, (void*)1);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2);
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  pi_reg_t pi_reg = {
		  .kp = 1.8f,
		  .ki = 400.0f,
		  .max_threshold = MAX_THRESHOLD_CONTROL_VOLTAGE,
		  .min_threshold = MIN_THRESHOLD_CONTROL_VOLTAGE,
		  .delta_t = 0.002,
		  .int_part = 0.0f
  };
  /* Infinite loop */
  for(;;)
  {
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	float real_milli_voltage = adc_buffer[1] * (3300.0f / 4095);
	float desired_milli_voltage = adc_buffer[0] * (3300.0f / 4095);
	float control_action = regulate(&pi_reg, desired_milli_voltage, real_milli_voltage);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, control_action);
	disp_real_milli_voltage = (uint16_t)(real_milli_voltage + 0.5f);
	disp_desired_milli_voltage = (uint16_t)(desired_milli_voltage + 0.5f);
  }
  /* USER CODE END ControlTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void millivolt_to_str(uint16_t mv, char *buf) {
    uint16_t whole = mv / 1000u;
    uint16_t frac  = (mv % 1000u) / 10u;

    if (whole >= 10) {
		buf[0] = '0' + (whole / 10);
		buf[1] = '0' + (whole % 10);
	} else {
		buf[0] = ' ';
		buf[1] = '0' + whole;
	}

	buf[2] = '.';
	buf[3] = '0' + (frac / 10);
	buf[4] = '0' + (frac % 10);
	buf[5] = '\0';
}

void OledTask(void const * argument)
{
	vTaskSetApplicationTaskTag(NULL, (void*)2);
	char buffer[10];
	ssd1306_Init();
	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("d=", Font_7x10, White);
	ssd1306_SetCursor(0, 30);
	ssd1306_WriteString("r=", Font_7x10, White);
	ssd1306_UpdateScreen();
	  /* Infinite loop */
	for(;;)
	{
	  while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
	  {
		  osDelay(1);
	  }
	  ssd1306_SetCursor(40, 0);
	  millivolt_to_str(disp_desired_milli_voltage, buffer);
	  ssd1306_WriteString(buffer, Font_7x10, White);
	  ssd1306_SetCursor(40, 30);
	  millivolt_to_str(disp_real_milli_voltage, buffer);
	  ssd1306_WriteString(buffer, Font_7x10, White);
	  ssd1306_UpdateScreen();
	  osDelay(100);
	}

}
/* USER CODE END Application */
