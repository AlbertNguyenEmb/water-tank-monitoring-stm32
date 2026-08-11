/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "logger.h"
#include "oled.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
void I2C_Scan(void);
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
    /* MCU Configuration------------------------------------------------------*/

    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();

    /* LED initially OFF */
    HAL_GPIO_WritePin(
        GPIOC,
        GPIO_PIN_13,
        GPIO_PIN_RESET
    );

    /* Initialize logger */
    Logger_Init();

    /* Give peripherals some time to stabilize */
    HAL_Delay(100);

    /* ---------------------------------------------------------------------- */
    /* I2C SCAN                                                              */
    /* ---------------------------------------------------------------------- */

    Logger_Print("\r\n");
    Logger_Print("================================\r\n");
    Logger_Print("       OLED TEST START\r\n");
    Logger_Print("================================\r\n");

    I2C_Scan();

    /* ---------------------------------------------------------------------- */
    /* OLED INIT                                                             */
    /* ---------------------------------------------------------------------- */

    Logger_Print("Initializing OLED...\r\n");

    OLED_Init();

    Logger_Print("OLED Init Done\r\n");

    /* ---------------------------------------------------------------------- */
    /* Initial OLED screen                                                   */
    /* ---------------------------------------------------------------------- */

    OLED_ShowLevel(65.0f);
    OLED_ShowStatus("FILLING");

    Logger_Print("OLED Display Test Started\r\n");

    /* Infinite loop --------------------------------------------------------*/
    while (1)
    {
        /* -------------------------------------------------------------- */
        /* Test 1: FILLING                                               */
        /* -------------------------------------------------------------- */

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        OLED_ShowLevel(35.5f);
        OLED_ShowStatus("FILLING");

        Logger_Print(
            "[OLED] Level = 35.5%% | State = FILLING\r\n"
        );

        HAL_Delay(2000);


        /* -------------------------------------------------------------- */
        /* Test 2: NORMAL                                                */
        /* -------------------------------------------------------------- */

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        OLED_ShowLevel(65.0f);
        OLED_ShowStatus("NORMAL");

        Logger_Print(
            "[OLED] Level = 65.0%% | State = NORMAL\r\n"
        );

        HAL_Delay(2000);


        /* -------------------------------------------------------------- */
        /* Test 3: FULL                                                  */
        /* -------------------------------------------------------------- */

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        OLED_ShowLevel(85.7f);
        OLED_ShowStatus("FULL");

        Logger_Print(
            "[OLED] Level = 85.7%% | State = FULL\r\n"
        );

        HAL_Delay(2000);


        /* -------------------------------------------------------------- */
        /* Test 4: ERROR                                                 */
        /* -------------------------------------------------------------- */

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        OLED_ShowLevel(-1.0f);
        OLED_ShowStatus("SENSOR ERR");

        Logger_Print(
            "[OLED] Level = ERROR | State = SENSOR ERR\r\n"
        );

        HAL_Delay(2000);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/**
  * @brief Scan all I2C addresses.
  */
void I2C_Scan(void)
{
    Logger_Print("I2C Scan Start\r\n");

    for (uint8_t address = 1; address < 128; address++)
    {
        if (HAL_I2C_IsDeviceReady(
                &hi2c1,
                address << 1,
                3,
                100) == HAL_OK)
        {
            Logger_Printf(
                "I2C Device Found: 0x%02X\r\n",
                address
            );
        }
    }

    Logger_Print("I2C Scan Done\r\n");
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    (void)file;
    (void)line;
    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */