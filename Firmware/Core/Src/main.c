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
#include "relay.h"
#include "buzzer.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "filter.h"
#include "fsm_logic.h"
#include "ultrasonic.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WATER_TANK_HEIGHT_CM             100.0f
#define ULTRASONIC_SAMPLE_PERIOD_MS      1000U
#define RUN_ULTRASONIC_UART_TEST         1U
#define RUN_STARTUP_SELF_TESTS           0U
#define RUN_BUZZER_STARTUP_TEST          0U

#define FSM_LOW_LEVEL_PERCENT            20.0f
#define FSM_FILL_STOP_PERCENT            90.0f
#define FSM_OVERFLOW_PERCENT             98.0f
#define FSM_OVERFLOW_CLEAR_PERCENT       95.0f
#define FSM_FILL_TIMEOUT_MS              120000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
#if !RUN_ULTRASONIC_UART_TEST
static DistanceFilter_t water_level_filter;
#endif
static uint32_t ultrasonic_last_sample_ms;
static UltrasonicStatus_t ultrasonic_last_reported_status;
static uint32_t ultrasonic_test_count;
#if !RUN_ULTRASONIC_UART_TEST
static FsmOutput_t water_fsm_output;
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
void I2C_Scan(void);
#if RUN_STARTUP_SELF_TESTS
static void Filter_Test_Run(void);
static void FSM_Test_Run(void);
#endif
#if RUN_ULTRASONIC_UART_TEST
static void Ultrasonic_Uart_Test_Init(void);
static bool Ultrasonic_Uart_Test_MeasureOnce(float *distance_cm);
static bool OLED_Debug_Run(uint8_t *oled_address);
#else
static void Ultrasonic_WaterLevel_Test_Init(void);
static void Ultrasonic_WaterLevel_Test_Task(void);
#endif
static void Buzzer_Test_Run(void);
#if !RUN_ULTRASONIC_UART_TEST
static void Apply_Fsm_Output(const FsmOutput_t *output);
#endif
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
    Relay_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();
    Logger_Init();
    Buzzer_Init();

    Buzzer_Test_Run();

#if RUN_ULTRASONIC_UART_TEST
    Ultrasonic_Uart_Test_Init();
    uint8_t oled_address = 0x3CU;
    if (OLED_Debug_Run(&oled_address))
    {
        OLED_SetI2CAddress(oled_address);
        if (OLED_Init())
        {
            OLED_ShowUltrasonicTest(0U, 0.0f, "INIT", false);
            Logger_Printf("OLED_Init done at 0x%02X\r\n", oled_address);
        }
        else
        {
            Logger_Printf("OLED_Init failed at 0x%02X\r\n", oled_address);
        }
    }
    else
    {
        Logger_Print("OLED_Init not run: OLED address not detected\r\n");
    }
#else
    if (!OLED_Init())
    {
        Logger_Print("OLED_Init failed\r\n");
    }

#if RUN_STARTUP_SELF_TESTS
    Filter_Test_Run();
    FSM_Test_Run();
#endif

    Ultrasonic_WaterLevel_Test_Init();
#endif

    /* Infinite loop --------------------------------------------------------*/
    while (1)
    {
#if RUN_ULTRASONIC_UART_TEST
        float distance_cm = 0.0f;

        ultrasonic_test_count++;

        Logger_Printf(
            "sample=%lu start\r\n",
            (unsigned long)ultrasonic_test_count
        );

        if (Ultrasonic_Uart_Test_MeasureOnce(&distance_cm))
        {
            Logger_Printf(
                "sample=%lu distance=%6.2f cm\r\n",
                (unsigned long)ultrasonic_test_count,
                distance_cm
            );
            OLED_ShowUltrasonicTest(
                ultrasonic_test_count,
                distance_cm,
                Ultrasonic_GetStatusName(Ultrasonic_GetStatus()),
                true
            );
        }
        else
        {
            Logger_Printf(
                "sample=%lu distance=ERROR status=%s\r\n",
                (unsigned long)ultrasonic_test_count,
                Ultrasonic_GetStatusName(Ultrasonic_GetStatus())
            );
            OLED_ShowUltrasonicTest(
                ultrasonic_test_count,
                0.0f,
                Ultrasonic_GetStatusName(Ultrasonic_GetStatus()),
                false
            );
        }

        HAL_Delay(ULTRASONIC_SAMPLE_PERIOD_MS);
#else
        Ultrasonic_WaterLevel_Test_Task();
        HAL_Delay(5);
#endif
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

#if RUN_STARTUP_SELF_TESTS
static float Test_AbsFloat(float value)
{
    if (value < 0.0f)
    {
        return -value;
    }

    return value;
}

static void Filter_Test_Run(void)
{
    typedef struct
    {
        float input_distance_cm;
        float expected_filtered_cm;
        float expected_water_level_percent;
        bool expected_valid;
        bool expected_ready;
        uint8_t expected_sample_count;
    } FilterTestCase_t;

    const float test_tank_height_cm = 100.0f;

    const FilterTestCase_t test_cases[] = {
        {20.1f, 20.10f, 79.90f, true, false, 1U},
        {20.3f, 20.20f, 79.80f, true, false, 2U},
        {19.8f, 20.07f, 79.93f, true, false, 3U},
        {20.7f, 20.23f, 79.77f, true, false, 4U},
        {20.0f, 20.18f, 79.82f, true, true, 5U},
        {21.1f, 20.38f, 79.62f, true, true, 5U},
        {0.0f, 20.38f, 79.62f, false, true, 5U},
        {401.0f, 20.38f, 79.62f, false, true, 5U},
    };

    DistanceFilter_t filter;
    float filtered_cm = 0.0f;
    float water_level_percent = 0.0f;
    uint8_t pass_count = 0U;
    uint8_t total_count = (uint8_t)(sizeof(test_cases) / sizeof(test_cases[0]));

    Filter_Init(&filter);
    Filter_SetTankHeightCm(&filter, test_tank_height_cm);

    Logger_Print("\r\nFilter test started\r\n");
    Logger_Printf(
        "Tank height: %6.2f cm\r\n",
        Filter_GetTankHeightCm(&filter)
    );
    Logger_Print("Input distance -> filtered distance -> water level\r\n");

    for (uint8_t i = 0U; i < total_count; i++)
    {
        bool valid = Filter_UpdateWaterLevel(
            &filter,
            test_cases[i].input_distance_cm,
            &filtered_cm,
            &water_level_percent
        );

        bool passed =
            (valid == test_cases[i].expected_valid) &&
            (Filter_IsReady(&filter) == test_cases[i].expected_ready) &&
            (Filter_GetSampleCount(&filter) == test_cases[i].expected_sample_count) &&
            (Test_AbsFloat(filtered_cm - test_cases[i].expected_filtered_cm) <= 0.02f) &&
            (Test_AbsFloat(water_level_percent - test_cases[i].expected_water_level_percent) <= 0.02f);

        if (passed)
        {
            pass_count++;
        }

        Logger_Printf(
            "T%02u %s | in=%6.2f cm | filtered=%6.2f cm | level=%6.2f%% | valid=%u ready=%u count=%u\r\n",
            (unsigned int)(i + 1U),
            passed ? "PASS" : "FAIL",
            test_cases[i].input_distance_cm,
            filtered_cm,
            water_level_percent,
            valid ? 1U : 0U,
            Filter_IsReady(&filter) ? 1U : 0U,
            (unsigned int)Filter_GetSampleCount(&filter)
        );
    }

    Filter_Reset(&filter);

    Logger_Printf(
        "After reset | ready=%u count=%u distance=%6.2f cm level=%6.2f%%\r\n",
        Filter_IsReady(&filter) ? 1U : 0U,
        (unsigned int)Filter_GetSampleCount(&filter),
        Filter_GetDistanceCm(&filter),
        Filter_GetWaterLevelPercent(&filter)
    );

    Logger_Printf(
        "Filter test done: %u/%u passed\r\n\r\n",
        pass_count,
        total_count
    );
}

static void FSM_Test_Run(void)
{
    typedef struct
    {
        FsmInput_t input;
        FsmState_t expected_state;
        FsmError_t expected_error;
        bool expected_pump_on;
        bool expected_buzzer_on;
    } FsmTestCase_t;

    const FsmTestCase_t test_cases[] = {
        {{50.0f, true, false, 0U}, FSM_STATE_MONITORING, FSM_ERROR_NONE, false, false},
        {{15.0f, true, false, 1000U}, FSM_STATE_FILLING, FSM_ERROR_NONE, true, false},
        {{50.0f, true, false, 2000U}, FSM_STATE_FILLING, FSM_ERROR_NONE, true, false},
        {{90.0f, true, false, 3000U}, FSM_STATE_MONITORING, FSM_ERROR_NONE, false, false},
        {{99.0f, true, false, 4000U}, FSM_STATE_OVERFLOW, FSM_ERROR_NONE, false, true},
        {{95.0f, true, false, 5000U}, FSM_STATE_MONITORING, FSM_ERROR_NONE, false, false},
        {{50.0f, false, false, 6000U}, FSM_STATE_ERROR, FSM_ERROR_SENSOR, false, true},
        {{50.0f, true, false, 7000U}, FSM_STATE_MONITORING, FSM_ERROR_NONE, false, false},
        {{15.0f, true, false, 8000U}, FSM_STATE_FILLING, FSM_ERROR_NONE, true, false},
        {{15.0f, true, false, 129000U}, FSM_STATE_ERROR, FSM_ERROR_FILL_TIMEOUT, false, true},
        {{50.0f, true, true, 130000U}, FSM_STATE_MONITORING, FSM_ERROR_NONE, false, false},
    };

    uint8_t pass_count = 0U;
    uint8_t total_count = (uint8_t)(sizeof(test_cases) / sizeof(test_cases[0]));

    Fsm_Init();

    Logger_Print("\r\nFSM logic test started\r\n");

    for (uint8_t i = 0U; i < total_count; i++)
    {
        FsmOutput_t output = Fsm_Update(&test_cases[i].input);

        bool passed =
            (output.state == test_cases[i].expected_state) &&
            (output.error == test_cases[i].expected_error) &&
            (output.pump_on == test_cases[i].expected_pump_on) &&
            (output.buzzer_on == test_cases[i].expected_buzzer_on);

        if (passed)
        {
            pass_count++;
        }

        Logger_Printf(
            "T%02u %s | level=%5.1f%% sensor=%u reset=%u | state=%s pump=%u buzzer=%u error=%s\r\n",
            (unsigned int)(i + 1U),
            passed ? "PASS" : "FAIL",
            test_cases[i].input.water_level_percent,
            test_cases[i].input.sensor_ok ? 1U : 0U,
            test_cases[i].input.user_reset ? 1U : 0U,
            Fsm_GetStateName(output.state),
            output.pump_on ? 1U : 0U,
            output.buzzer_on ? 1U : 0U,
            Fsm_GetErrorName(output.error)
        );
    }

    Logger_Printf(
        "FSM logic test done: %u/%u passed\r\n\r\n",
        pass_count,
        total_count
    );
}
#endif

#if RUN_ULTRASONIC_UART_TEST
static bool OLED_Debug_Run(uint8_t *oled_address)
{
    bool oled_found = false;
    GPIO_PinState scl_state;
    GPIO_PinState sda_state;

    Logger_Print("\r\nOLED/I2C debug started\r\n");
    Logger_Print("I2C1 pins: PB6=SCL PB7=SDA\r\n");
    scl_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
    sda_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7);
    Logger_Printf(
        "Idle pins: SCL=%s SDA=%s\r\n",
        (scl_state == GPIO_PIN_SET) ? "HIGH" : "LOW",
        (sda_state == GPIO_PIN_SET) ? "HIGH" : "LOW"
    );
    Logger_Print("Scanning I2C bus...\r\n");

    for (uint8_t address = 1U; address < 128U; address++)
    {
        if (HAL_I2C_IsDeviceReady(
                &hi2c1,
                (uint16_t)(address << 1),
                2,
                20) == HAL_OK)
        {
            Logger_Printf(
                "I2C device found: 0x%02X\r\n",
                address
            );

            if ((address == 0x3CU) || (address == 0x3DU))
            {
                oled_found = true;
                if (oled_address != NULL)
                {
                    *oled_address = address;
                }
            }
        }
    }

    if (HAL_I2C_IsDeviceReady(
            &hi2c1,
            (uint16_t)(0x3CU << 1),
            3,
            100) == HAL_OK)
    {
        oled_found = true;
        if (oled_address != NULL)
        {
            *oled_address = 0x3CU;
        }
        Logger_Print("OLED check 0x3C: OK\r\n");
    }
    else
    {
        Logger_Print("OLED check 0x3C: FAIL\r\n");
    }

    if (HAL_I2C_IsDeviceReady(
            &hi2c1,
            (uint16_t)(0x3DU << 1),
            3,
            100) == HAL_OK)
    {
        if ((oled_address != NULL) && !oled_found)
        {
            *oled_address = 0x3DU;
        }
        oled_found = true;
        Logger_Print("OLED check 0x3D: OK\r\n");
    }
    else
    {
        Logger_Print("OLED check 0x3D: FAIL\r\n");
    }

    if (oled_found)
    {
        Logger_Printf(
            "OLED hardware detected at 0x%02X. Calling OLED_Init...\r\n",
            (oled_address != NULL) ? *oled_address : 0x3CU
        );
    }
    else
    {
        Logger_Print("OLED not detected. Check VCC/GND/SCL/SDA/address/pull-up.\r\n");
    }

    Logger_Print("OLED/I2C debug done\r\n\r\n");

    return oled_found;
}

static void Ultrasonic_Uart_Test_Init(void)
{
    Ultrasonic_Init();

    ultrasonic_last_sample_ms = HAL_GetTick() - ULTRASONIC_SAMPLE_PERIOD_MS;
    ultrasonic_last_reported_status = ULTRASONIC_STATUS_IDLE;
    ultrasonic_test_count = 0U;

    Logger_Print("\r\nUltrasonic UART test started\r\n");
    Logger_Printf(
        "TRIG=PA1 GPIO | ECHO=PA0 TIM2_CH1 | period=%u ms\r\n",
        (unsigned int)ULTRASONIC_SAMPLE_PERIOD_MS
    );
    Logger_Printf(
        "ECHO idle: PA0=%s\r\n",
        (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? "HIGH" : "LOW"
    );
}

static bool Ultrasonic_Uart_Test_MeasureOnce(float *distance_cm)
{
    return Ultrasonic_MeasureDistanceCm(distance_cm);
}
#else

static void Ultrasonic_WaterLevel_Test_Init(void)
{
    FsmConfig_t fsm_config = {
        FSM_LOW_LEVEL_PERCENT,
        FSM_FILL_STOP_PERCENT,
        FSM_OVERFLOW_PERCENT,
        FSM_OVERFLOW_CLEAR_PERCENT,
        FSM_FILL_TIMEOUT_MS
    };

    Ultrasonic_Init();
    Fsm_InitWithConfig(&fsm_config);

    Filter_Init(&water_level_filter);
    Filter_SetTankHeightCm(&water_level_filter, WATER_TANK_HEIGHT_CM);

    ultrasonic_last_sample_ms = HAL_GetTick() - ULTRASONIC_SAMPLE_PERIOD_MS;
    ultrasonic_last_reported_status = ULTRASONIC_STATUS_IDLE;
    water_fsm_output.state = FSM_STATE_INIT;
    water_fsm_output.error = FSM_ERROR_NONE;
    water_fsm_output.pump_on = false;
    water_fsm_output.buzzer_on = false;

    OLED_ShowLevel(-1.0f);
    OLED_ShowStatus("INIT");

    Logger_Print("\r\nWater tank monitor started\r\n");
    Logger_Printf(
        "TRIG=PA1 GPIO | ECHO=PA0 TIM2_CH1 | tank_height=%6.2f cm | period=%u ms\r\n",
        Filter_GetTankHeightCm(&water_level_filter),
        (unsigned int)ULTRASONIC_SAMPLE_PERIOD_MS
    );
    Logger_Printf(
        "FSM low=%4.1f%% fill_stop=%4.1f%% overflow=%4.1f%% clear=%4.1f%% timeout=%lu ms\r\n",
        FSM_LOW_LEVEL_PERCENT,
        FSM_FILL_STOP_PERCENT,
        FSM_OVERFLOW_PERCENT,
        FSM_OVERFLOW_CLEAR_PERCENT,
        (unsigned long)FSM_FILL_TIMEOUT_MS
    );
}

static void Ultrasonic_WaterLevel_Test_Task(void)
{
    uint32_t now_ms = HAL_GetTick();
    UltrasonicStatus_t status;
    float raw_distance_cm = 0.0f;
    float filtered_distance_cm = 0.0f;
    float water_level_percent = 0.0f;

    Ultrasonic_Process();

    if (Ultrasonic_ReadDistanceCm(&raw_distance_cm))
    {
        bool filter_valid = Filter_UpdateWaterLevel(
            &water_level_filter,
            raw_distance_cm,
            &filtered_distance_cm,
            &water_level_percent
        );
        FsmInput_t fsm_input = {
            water_level_percent,
            filter_valid,
            false,
            now_ms
        };

        water_fsm_output = Fsm_Update(&fsm_input);
        Apply_Fsm_Output(&water_fsm_output);

        OLED_ShowLevel(water_level_percent);
        OLED_ShowStatus(Fsm_GetStateName(water_fsm_output.state));

        Logger_Printf(
            "WATER | raw=%6.2f cm | filtered=%6.2f cm | level=%6.2f%% | state=%s | pump=%u buzzer=%u error=%s | echo=%lu us | filter=%u ready=%u count=%u\r\n",
            raw_distance_cm,
            filtered_distance_cm,
            water_level_percent,
            Fsm_GetStateName(water_fsm_output.state),
            water_fsm_output.pump_on ? 1U : 0U,
            water_fsm_output.buzzer_on ? 1U : 0U,
            Fsm_GetErrorName(water_fsm_output.error),
            (unsigned long)Ultrasonic_GetEchoTimeUs(),
            filter_valid ? 1U : 0U,
            Filter_IsReady(&water_level_filter) ? 1U : 0U,
            (unsigned int)Filter_GetSampleCount(&water_level_filter)
        );
    }

    status = Ultrasonic_GetStatus();
    if (((status == ULTRASONIC_STATUS_TIMEOUT) ||
         (status == ULTRASONIC_STATUS_ERROR)) &&
        (status != ultrasonic_last_reported_status))
    {
        FsmInput_t fsm_input = {
            Filter_GetWaterLevelPercent(&water_level_filter),
            false,
            false,
            now_ms
        };

        water_fsm_output = Fsm_Update(&fsm_input);
        Apply_Fsm_Output(&water_fsm_output);

        OLED_ShowLevel(-1.0f);
        OLED_ShowStatus(Fsm_GetErrorName(water_fsm_output.error));

        Logger_Printf(
            "WATER | sensor=%s | state=%s | pump=%u buzzer=%u error=%s\r\n",
            Ultrasonic_GetStatusName(status),
            Fsm_GetStateName(water_fsm_output.state),
            water_fsm_output.pump_on ? 1U : 0U,
            water_fsm_output.buzzer_on ? 1U : 0U,
            Fsm_GetErrorName(water_fsm_output.error)
        );
        ultrasonic_last_reported_status = status;
    }

    if (!Ultrasonic_IsBusy() &&
        ((uint32_t)(now_ms - ultrasonic_last_sample_ms) >=
         ULTRASONIC_SAMPLE_PERIOD_MS))
    {
        ultrasonic_last_sample_ms = now_ms;

        if (Ultrasonic_Start())
        {
            ultrasonic_last_reported_status = ULTRASONIC_STATUS_WAIT_RISING;
        }
        else
        {
            Logger_Printf(
                "US START FAIL | status=%s\r\n",
                Ultrasonic_GetStatusName(Ultrasonic_GetStatus())
            );
        }
    }
}
#endif

static void Buzzer_Test_Run(void)
{
#if RUN_BUZZER_STARTUP_TEST
    Logger_Print("\r\nBuzzer test started\r\n");

    for (uint8_t i = 0U; i < 3U; i++)
    {
        Buzzer_On();
        Logger_Printf(
            "Buzzer T%02u ON\r\n",
            (unsigned int)(i + 1U)
        );
        HAL_Delay(150);

        Buzzer_Off();
        Logger_Printf(
            "Buzzer T%02u OFF\r\n",
            (unsigned int)(i + 1U)
        );
        HAL_Delay(150);
    }

    Logger_Print("Buzzer test done\r\n\r\n");
#else
    Buzzer_Off();
#endif
}

#if !RUN_ULTRASONIC_UART_TEST
static void Apply_Fsm_Output(const FsmOutput_t *output)
{
    if (output == NULL)
    {
        Relay_Off();
        Buzzer_Set(false);
        return;
    }

    if (output->pump_on)
    {
        Relay_On();
    }
    else
    {
        Relay_Off();
    }

    Buzzer_Set(output->buzzer_on);
}
#endif

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
