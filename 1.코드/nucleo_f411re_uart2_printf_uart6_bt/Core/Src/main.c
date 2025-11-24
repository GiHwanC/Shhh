/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "my_lcd.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
#define ARR_CNT 15
#define CMD_SIZE 50
#define LED_PIN GPIO_PIN_5
#define LED_PORT GPIOB
uint8_t ledBlinkState = 0;      // LED 깜박임 상태
uint32_t ledBlinkStart = 0;     // LED 깜박 시작 시간
uint32_t lastBlinkTick = 0;     // 마지막 토글 시간
#define BLINK_INTERVAL 1000     // 1초 간격
#define BLINK_DURATION 5000     // 5초 동안 깜박임

#define BLINK_INTERVAL 500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
volatile unsigned char btFlag = 0;
uint8_t btchar;
char btData[50];
volatile uint32_t lastTick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
void bluetooth_Event();
void loop_LED_Blink();
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
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

		lcd_init(&hi2c2);

		lcd_clear();
		HAL_Delay(50);
		lcd_set_cursor(0, 0);
		lcd_send_string("ROOM:");

		HAL_UART_Receive_IT(&huart6, &btchar, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

		if (btFlag) {
			btFlag = 0;
			bluetooth_Event();

		}
		loop_LED_Blink();


		if (HAL_GetTick() - lastTick >= 500) {
			const char req[] = "[KGH_SQL]GETDB@ROOM@1\r\n";
			HAL_StatusTypeDef st = HAL_UART_Transmit(&huart6, (uint8_t*) req,
					sizeof(req) - 1, 1000);
			lastTick = HAL_GetTick();
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  huart6.Init.BaudRate = 9600;
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void MX_GPIO_LED_ON(int pin) {
	HAL_GPIO_WritePin(LD2_GPIO_Port, pin, GPIO_PIN_SET);
}
void MX_GPIO_LED_OFF(int pin) {
	HAL_GPIO_WritePin(LD2_GPIO_Port, pin, GPIO_PIN_RESET);
}
void loop_LED_Blink(){
	 if (ledBlinkState) {
	        uint32_t now = HAL_GetTick();

	        // 5초 동안 깜박임
	        if (now - ledBlinkStart <= BLINK_DURATION) {
	            if (now - lastBlinkTick >= BLINK_INTERVAL) {
	                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
	                lastBlinkTick = now;
	            }
	        } else {
	            // 5초 지나면 LED 끄기
	            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
	            ledBlinkState = 0;
	        }
	    }
}
void bluetooth_Event() {

	int i = 0;
	char *pToken;
	char *pArray[ARR_CNT] = { 0 };
	char recvBuf[CMD_SIZE] = { 0 };
	strcpy(recvBuf, btData);

	pToken = strtok(recvBuf, "[@]");
	while (pToken != NULL) {
		pArray[i] = pToken;
		if (++i >= ARR_CNT)
			break;
		pToken = strtok(NULL, "[@]");
	}
	if (i >= 4 && strcmp(pArray[1], "GETDB") == 0
			&& strcmp(pArray[2], "ROOM") == 0) {
		int gas = -1, fire = -1, sound = -1;

		// 키-값 쌍 순회하며 값 저장
		 for (int j = 2; j < i - 1; j += 2) {
		            if (strcmp(pArray[j], "GAS") == 0) {
		                gas = atoi(pArray[j + 1]);
		            } else if (strcmp(pArray[j], "FIRE") == 0) {
		                fire = atoi(pArray[j + 1]);
		            } else if (strcmp(pArray[j], "SOUND") == 0) {
		                sound = atoi(pArray[j + 1]);
		            }
		        }

		        // LCD 출력 조건

		 int condition = 0;

		         if (gas > 400) {
		             lcd_set_cursor(1, 7);
		             lcd_send_string("1_G");
		             condition = 1;
		         }
		         if (fire > 100) {
		             lcd_set_cursor(0, 7);
		             lcd_send_string("1_F");
		             condition = 1;
		         }
		         if (sound > 700) {
		             lcd_set_cursor(0, 11);
		             lcd_send_string("1_S");
		             condition = 1;
		         }

		         if (condition) {
		             // LED 깜박임 시작
		             ledBlinkState = 1;
		             ledBlinkStart = HAL_GetTick();
		             lastBlinkTick = HAL_GetTick();
		         } else {
		             // 조건 만족 안 하면 LCD 초기화 후 기본 표시
		             lcd_clear();
		             lcd_set_cursor(0, 0);
		             lcd_send_string("ROOM:");

		             // LED 끄기
		             HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
		             ledBlinkState = 0;
		         }


		     if(gas>400){
		 	lcd_set_cursor(1, 7);
		 				lcd_send_string("1_G");


		 }
		 else{
			 lcd_set_cursor(1,7);
			 lcd_send_string("   ");
		 }
		 		// LCD 출력 조건

		// LCD 출력 조건
		if (fire<100) {

			lcd_set_cursor(0, 7);
			lcd_send_string("1_F");


		} else{
			 lcd_set_cursor(0,7);
			 lcd_send_string("   ");
		 }
		if (sound>700) {

			lcd_set_cursor(0, 11);
			lcd_send_string("1_S");


		} else{
			 lcd_set_cursor(0,11);
			 lcd_send_string("   ");
		 }
		if ((gas > 400) || (fire < 100) || (sound>700)) {
		            ledBlinkState = 1;
		            ledBlinkStart = HAL_GetTick();   // 현재 시간 저장
		            lastBlinkTick = HAL_GetTick();   // 토글 초기화
		        }


}


}
/**
 * @brief  Retargets the C library printf function to the USART.
 * @param  None
 * @retval None
 */
PUTCHAR_PROTOTYPE {
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART6 and Loop until the end of transmission */
	HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 0xFFFF);

	return ch;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART6) {
		static int i = 0;
		btData[i] = btchar;
		if ((btData[i] == '\n') || btData[i] == '\r') {
			btData[i] = '\0';
			btFlag = 1;
			i = 0;
		} else {
			i++;
		}
		HAL_UART_Receive_IT(&huart6, &btchar, 1);
	}
}
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
	while (1) {
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
