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
#include "uart_drv.h"
#include "button.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    ST_OP1 = 0,
    ST_OP,
    ST_OP2,
    ST_EQ
} calc_state_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RED_LED_PORT   GPIOD
#define RED_LED_PIN    GPIO_PIN_15
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
static calc_state_t s_state;
static int32_t      s_op1;
static int32_t      s_op2;
static char         s_op;
static int32_t      s_cur;
static uint8_t      s_digits;

static void red_led(int on)
{
    HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void calc_reset(void)
{
    s_state  = ST_OP1;
    s_op1    = 0;
    s_op2    = 0;
    s_op     = 0;
    s_cur    = 0;
    s_digits = 0;
}

static void print_prompt(void) { uart_send_str("\r\n> "); }

static void print_int(int32_t v)
{
    char buf[8];
    int  i = 0;
    int  neg = 0;

    if (v == 0) { uart_send_byte('0'); return; }
    if (v < 0) { neg = 1; v = -v; }

    while (v > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    if (neg) uart_send_byte('-');
    while (i > 0) uart_send_byte((uint8_t)buf[--i]);
}

static void report_error(void)
{
    uart_send_str("\r\nerror");
    red_led(1);
    calc_reset();
    print_prompt();
}

static void print_mode_banner(uart_mode_t m)
{
    if (m == UART_MODE_IRQ) {
        uart_send_str("\r\n[Mode: INTERRUPTS ON]");
    } else {
        uart_send_str("\r\n[Mode: POLLING]");
    }
    uart_send_str("\r\nInput cleared. Start typing again.");
    print_prompt();
}

static void do_compute(void)
{
    int32_t r   = 0;
    int     err = 0;

    switch (s_op) {
        case '+': r = s_op1 + s_op2; break;
        case '-': r = s_op1 - s_op2; break;
        case '*': r = s_op1 * s_op2; break;
        case '/':
            if (s_op2 == 0) { err = 1; break; }
            r = s_op1 / s_op2;
            break;
        default: err = 1; break;
    }

    if (!err && (r > 32767 || r < -32768)) err = 1;

    if (err) {
        report_error();
    } else {
        uart_send_str("\r\n");
        print_int(r);
        red_led(0);
        calc_reset();
        print_prompt();
    }
}

static void handle_digit(char c)
{
    if (s_state == ST_OP1 && s_digits == 0) red_led(0);

    if (s_state == ST_OP || s_state == ST_EQ) { report_error(); return; }

    s_cur = s_cur * 10 + (c - '0');
    s_digits++;

    if (s_cur > 32767) { report_error(); return; }

    if (s_digits >= 5) {
        if (s_state == ST_OP1) {
            s_op1 = s_cur; s_cur = 0; s_digits = 0; s_state = ST_OP;
        } else {
            s_op2 = s_cur; s_cur = 0; s_digits = 0; s_state = ST_EQ;
        }
    }
}

static void handle_operator(char c)
{
    if (s_state == ST_OP1) {
        if (s_digits == 0) { report_error(); return; }
        s_op1 = s_cur; s_cur = 0; s_digits = 0;
        s_op = c; s_state = ST_OP2;
    } else if (s_state == ST_OP) {
        s_op = c; s_state = ST_OP2;
    } else {
        report_error();
    }
}

static void handle_equal(void)
{
    if (s_state == ST_OP2) {
        if (s_digits == 0) { report_error(); return; }
        s_op2 = s_cur; s_cur = 0; s_digits = 0; s_state = ST_EQ;
    }
    if (s_state == ST_EQ) {
        do_compute();
    } else {
        report_error();
    }
}

static void handle_byte(uint8_t b)
{
    if (b == 0x08 || b == 0x7F) {
        if (s_digits > 0) {
            uart_send_str("\b \b");
            s_cur /= 10;
            s_digits--;
        }
        return;
    }

    uart_send_byte(b);

    if (b >= '0' && b <= '9')                       handle_digit((char)b);
    else if (b=='+' || b=='-' || b=='*' || b=='/')  handle_operator((char)b);
    else if (b == '=')                              handle_equal();
    else if (b == '\r' || b == '\n')                { /* ignore */ }
    else                                            report_error();
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART6_UART_Init(void);
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

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
    uart_init();
    uart_set_mode(UART_MODE_POLL);
    button_init();

    calc_reset();
    red_led(0);

    print_mode_banner(UART_MODE_POLL);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      button_event_t ev = button_poll();
      if (ev == BTN_SHORT_PRESS) {
          uart_mode_t m = (uart_get_mode() == UART_MODE_IRQ)
                          ? UART_MODE_POLL
                          : UART_MODE_IRQ;
          uart_set_mode(m);
          calc_reset();
          red_led(0);
          print_mode_banner(m);
      }

      uint8_t b;
      while (uart_recv_byte(&b)) {
          handle_byte(b);
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
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 19200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
