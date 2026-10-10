/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button.h"
#include "leds.h"
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

/* USER CODE BEGIN PV */
/* --- Прикладное состояние --- */
static int s_counter        = 0;   /* 0..3 */
static int s_overflow_count = 0;

/* --- Конечный автомат анимации --- */
#define ANIM_STEP_MS 150u

typedef enum {
    ANIM_IDLE = 0,
    ANIM_DUAL_ON,
    ANIM_DUAL_OFF,
    ANIM_GREEN_ON,
    ANIM_GREEN_OFF
} anim_state_t;

static anim_state_t s_anim       = ANIM_IDLE;
static uint32_t     s_anim_tick  = 0;
static int          s_anim_green = 0;   /* сколько ещё зелёных миганий */

static int anim_is_idle(void) { return s_anim == ANIM_IDLE; }

static void anim_start(int green_blinks)
{
    s_anim_green = green_blinks;
    leds_write(LED_ALL);          /* оба ON — старт «двойного» мигания */
    s_anim      = ANIM_DUAL_ON;
    s_anim_tick = HAL_GetTick();
}

/* Неблокирующий шаг анимации. Вызывать каждый проход цикла. */
static void anim_tick(void)
{
    if (s_anim == ANIM_IDLE) return;

    uint32_t now = HAL_GetTick();
    if ((now - s_anim_tick) < ANIM_STEP_MS) return;
    s_anim_tick = now;

    switch (s_anim) {
    case ANIM_DUAL_ON:
        leds_write(0);
        s_anim = ANIM_DUAL_OFF;
        break;

    case ANIM_DUAL_OFF:
        if (s_anim_green > 0) {
            leds_write(LED_GREEN);
            s_anim = ANIM_GREEN_ON;
        } else {
            s_anim = ANIM_IDLE;
        }
        break;

    case ANIM_GREEN_ON:
        leds_write(0);
        s_anim = ANIM_GREEN_OFF;
        break;

    case ANIM_GREEN_OFF:
        s_anim_green--;
        if (s_anim_green > 0) {
            leds_write(LED_GREEN);
            s_anim = ANIM_GREEN_ON;
        } else {
            s_anim = ANIM_IDLE;
        }
        break;

    default:
        s_anim = ANIM_IDLE;
        break;
    }
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
////
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
////
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
////
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  leds_init();
  button_init();
  leds_write((uint8_t)s_counter);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* --- Процесс 1: опрос кнопки (быстрый, неблокирующий) --- */
      button_event_t ev = button_poll();

      /* Реагируем только когда анимация не идёт, чтобы не «затереть» её */
      if (anim_is_idle()) {
          if (ev == BTN_SHORT_PRESS) {
              s_counter++;
              if (s_counter > 3) {
                  s_counter = 0;
                  s_overflow_count++;
                  anim_start(s_overflow_count);
              }
          } else if (ev == BTN_LONG_PRESS) {
              s_counter--;
              if (s_counter < 0) {
                  s_counter = 3;
                  if (s_overflow_count > 0) {
                      s_overflow_count--;
                      anim_start(s_overflow_count);
                  }
              }
          }
      }

      /* --- Процесс 2: шаг анимации (тоже неблокирующий) --- */
      anim_tick();

      /* --- Процесс 3: обновление LED, если нет анимации --- */
      if (anim_is_idle()) {
          leds_write((uint8_t)s_counter);
      }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
