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
#include "filter.h"
#include "logger.h"
#include "Oled.h"
#include "relay.h"
#include "ultrasonic.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WATER_TANK_HEIGHT_CM        50.0f
#define WATER_TANK_SAMPLE_DELAY_MS  200U
#define OLED_INIT_RETRY_COUNT       3U
#define OLED_INIT_RETRY_DELAY_MS    100U
#define OLED_RETRY_PERIOD_SAMPLES   100U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static DistanceFilter_t distance_filter;
static uint32_t sample_count = 0U;
static bool oled_ready = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
void I2C_Scan(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#ifdef ULTRASONIC_ECHO_TIMER
#define App_UltrasonicReadDistance(distance_cm) \
    Ultrasonic_MeasureDistanceCm(distance_cm)
#define App_UltrasonicGetEchoTimeUs() \
    Ultrasonic_GetEchoTimeUs()
#else
#define App_UltrasonicReadDistance(distance_cm) \
    Ultrasonic_ReadDistance(distance_cm)
#define App_UltrasonicGetEchoTimeUs() \
    0U
#endif

static bool App_InitOledWithRetry(void)
{
    for (uint8_t attempt = 0U; attempt < OLED_INIT_RETRY_COUNT; attempt++)
    {
        if (OLED_Init())
        {
            return true;
        }

        HAL_Delay(OLED_INIT_RETRY_DELAY_MS);
    }

    return false;
}
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
    Relay_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();

    Relay_Off();

    Logger_Print("===== WATER TANK TEST =====\r\n");
    I2C_Scan();

    HAL_Delay(100U);

    oled_ready = App_InitOledWithRetry();
    if (!oled_ready)
    {
        Logger_Print("OLED INIT FAILED!\r\n");
    }
    else
    {
        Logger_Print("OLED INIT OK!\r\n");
    }

    Ultrasonic_Init();
    Filter_Init(&distance_filter);
    (void)Filter_SetTankHeightCm(
        &distance_filter,
        WATER_TANK_HEIGHT_CM
    );

    if (oled_ready)
    {
        OLED_ShowLevel(-1.0f);
        OLED_ShowStatus("START");
    }

    /* Infinite loop --------------------------------------------------------*/
    while (1)
    {
        float raw_distance_cm = 0.0f;
        float filtered_distance_cm = 0.0f;
        float water_level_percent = 0.0f;

        sample_count++;

        if (!oled_ready && ((sample_count % OLED_RETRY_PERIOD_SAMPLES) == 0U))
        {
            oled_ready = App_InitOledWithRetry();
            Logger_Print(oled_ready ? "OLED RETRY OK\r\n" : "OLED RETRY FAILED\r\n");
        }

        if (App_UltrasonicReadDistance(&raw_distance_cm))
        {
            if (Filter_UpdateWaterLevel(
                    &distance_filter,
                    raw_distance_cm,
                    &filtered_distance_cm,
                    &water_level_percent))
            {
                Logger_Printf(
                    "Sample:%lu | ECHO=%lu us | RAW=%.1f cm | FILTER=%.1f cm | LEVEL=%.1f %%\r\n",
                    (unsigned long)sample_count,
                    (unsigned long)App_UltrasonicGetEchoTimeUs(),
                    raw_distance_cm,
                    filtered_distance_cm,
                    water_level_percent
                );

                if (oled_ready)
                {
                    OLED_ShowLevel(water_level_percent);
                    OLED_ShowStatus(Filter_IsReady(&distance_filter) ? "LEVEL OK" : "WARMUP");
                }
            }
            else
            {
                Logger_Printf(
                    "Sample:%lu | FILTER REJECT | ECHO=%lu us | RAW=%.1f cm\r\n",
                    (unsigned long)sample_count,
                    (unsigned long)App_UltrasonicGetEchoTimeUs(),
                    raw_distance_cm
                );
            }
        }
        else
        {
#ifdef ULTRASONIC_ECHO_TIMER
            Logger_Printf(
                "Sample:%lu | ULTRASONIC %s | ECHO=%lu us\r\n",
                (unsigned long)sample_count,
                Ultrasonic_GetStatusName(Ultrasonic_GetStatus()),
                (unsigned long)Ultrasonic_GetEchoTimeUs()
            );
#else
            Logger_Printf(
                "Sample:%lu | ULTRASONIC ERROR\r\n",
                (unsigned long)sample_count
            );
#endif

            if (Filter_GetSampleCount(&distance_filter) > 0U)
            {
                if (oled_ready)
                {
                    OLED_ShowLevel(Filter_GetWaterLevelPercent(&distance_filter));
                    OLED_ShowStatus("US RETRY");
                }
            }
            else
            {
                if (oled_ready)
                {
                    OLED_ShowLevel(-1.0f);
                    OLED_ShowStatus("US ERROR");
                }
            }
        }

        HAL_Delay(WATER_TANK_SAMPLE_DELAY_MS);
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
