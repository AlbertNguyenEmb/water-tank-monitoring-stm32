/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Water tank monitor application integration.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "logger.h"
#include "Oled.h"
#include "relay.h"
#include "buzzer.h"

#include "filter.h"
#include "fsm_logic.h"
#include "ultrasonic.h"

/* Private defines -----------------------------------------------------------*/
#define WATER_TANK_HEIGHT_CM             40.0f
#define ULTRASONIC_SAMPLE_PERIOD_MS      1000U

/* Production integration mode. */
#define RUN_STARTUP_SELF_TESTS           0U
#define RUN_BUZZER_STARTUP_TEST          0U

/* FSM configuration. */
#define FSM_LOW_LEVEL_PERCENT            20.0f
#define FSM_FILL_STOP_PERCENT            90.0f
#define FSM_OVERFLOW_PERCENT             98.0f
#define FSM_OVERFLOW_CLEAR_PERCENT       95.0f
#define FSM_FILL_TIMEOUT_MS              120000U

/* Sensor/filter behaviour. */
#define FSM_REQUIRE_FILTER_READY         1U

/* Private variables ---------------------------------------------------------*/
static DistanceFilter_t water_level_filter;
static FsmOutput_t water_fsm_output;

static uint32_t ultrasonic_last_sample_ms;
static UltrasonicStatus_t ultrasonic_last_reported_status;

/* Button PB5 -> reset FSM after an error.  Set from EXTI callback, consumed
 * in the main loop. */
static volatile bool fsm_reset_request;
static volatile uint32_t fsm_last_reset_button_ms;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#if RUN_STARTUP_SELF_TESTS
static void Filter_Test_Run(void);
static void FSM_Test_Run(void);
#endif

static void WaterLevel_Init(void);
static void WaterLevel_Task(void);
static void Apply_Fsm_Output(const FsmOutput_t *output);
static void Log_Fsm_Config(const FsmConfig_t *config);

/* Application entry point ---------------------------------------------------*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();

    Logger_Init();
    Relay_Init();
    Buzzer_Init();

    /* Safe startup outputs. */
    Relay_Off();
    Buzzer_Off();

#if RUN_BUZZER_STARTUP_TEST
    Buzzer_On();
    HAL_Delay(150);
    Buzzer_Off();
#endif

    if (!OLED_Init())
    {
        Logger_Print("ERROR | OLED_Init failed; continuing without display\r\n");
    }

#if RUN_STARTUP_SELF_TESTS
    Filter_Test_Run();
    FSM_Test_Run();
#endif

    WaterLevel_Init();

    Logger_Print("\r\n=== WATER TANK MONITOR START ===\r\n");

    while (1)
    {
        WaterLevel_Task();
        HAL_Delay(5U);
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

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

static void Log_Fsm_Config(const FsmConfig_t *config)
{
    if (config == NULL)
    {
        return;
    }

    Logger_Printf(
        "FSM | low=%4.1f%% fill_stop=%4.1f%% overflow=%4.1f%% "
        "clear=%4.1f%% timeout=%lu ms\r\n",
        config->low_level_percent,
        config->fill_stop_percent,
        config->overflow_percent,
        config->overflow_clear_percent,
        (unsigned long)config->fill_timeout_ms
    );
}

static void WaterLevel_Init(void)
{
    const FsmConfig_t fsm_config = {
        FSM_LOW_LEVEL_PERCENT,
        FSM_FILL_STOP_PERCENT,
        FSM_OVERFLOW_PERCENT,
        FSM_OVERFLOW_CLEAR_PERCENT,
        FSM_FILL_TIMEOUT_MS
    };

    fsm_reset_request = false;
    fsm_last_reset_button_ms = HAL_GetTick() - 150U;

    Filter_Init(&water_level_filter);
    if (!Filter_SetTankHeightCm(&water_level_filter, WATER_TANK_HEIGHT_CM))
    {
        Logger_Print("ERROR | invalid tank height configuration\r\n");
    }

    Fsm_InitWithConfig(&fsm_config);

    Ultrasonic_Init();

    water_fsm_output = (FsmOutput_t){
        FSM_STATE_INIT,
        FSM_ERROR_NONE,
        false,
        false
    };

    ultrasonic_last_sample_ms = HAL_GetTick() - ULTRASONIC_SAMPLE_PERIOD_MS;
    ultrasonic_last_reported_status = ULTRASONIC_STATUS_IDLE;

    Apply_Fsm_Output(&water_fsm_output);
    OLED_ShowLevel(-1.0f);
    OLED_ShowStatus("INIT");

    Logger_Printf(
        "US | TRIG=PA1 | ECHO=PA0 TIM2_CH1 | tank_height=%6.2f cm | period=%u ms\r\n",
        Filter_GetTankHeightCm(&water_level_filter),
        (unsigned int)ULTRASONIC_SAMPLE_PERIOD_MS
    );
    Log_Fsm_Config(&fsm_config);
    Logger_Print("BTN | PB5 active-low reset input\r\n");
}

static void WaterLevel_Task(void)
{
    const uint32_t now_ms = HAL_GetTick();
    UltrasonicStatus_t status;
    float raw_distance_cm = 0.0f;
    float filtered_distance_cm = Filter_GetDistanceCm(&water_level_filter);
    float water_level_percent = Filter_GetWaterLevelPercent(&water_level_filter);
    bool distance_processed = false;

    Ultrasonic_Process();

    /* Consume a reset request generated by the EXTI callback. */
    const bool user_reset = fsm_reset_request;
    fsm_reset_request = false;

    if (Ultrasonic_ReadDistanceCm(&raw_distance_cm))
    {
        const bool filter_valid = Filter_UpdateWaterLevel(
            &water_level_filter,
            raw_distance_cm,
            &filtered_distance_cm,
            &water_level_percent
        );

        const bool filter_ready = Filter_IsReady(&water_level_filter);
        const bool sensor_ok = filter_valid && filter_ready;

#if FSM_REQUIRE_FILTER_READY
        /* Do not let the FSM react to the first few raw samples.  The filter
         * must have a complete window before its level is trusted. */
        if (filter_ready)
        {
            FsmInput_t fsm_input = {
                water_level_percent,
                sensor_ok,
                user_reset,
                now_ms
            };

            water_fsm_output = Fsm_Update(&fsm_input);
            Apply_Fsm_Output(&water_fsm_output);
            OLED_ShowLevel(water_level_percent);
            OLED_ShowStatus(Fsm_GetStateName(water_fsm_output.state));
        }
        else
        {
            water_fsm_output = (FsmOutput_t){
                FSM_STATE_INIT,
                FSM_ERROR_NONE,
                false,
                false
            };
            Apply_Fsm_Output(&water_fsm_output);
            OLED_ShowLevel(-1.0f);
            OLED_ShowStatus("FILTERING");
        }
#else
        FsmInput_t fsm_input = {
            water_level_percent,
            filter_valid,
            user_reset,
            now_ms
        };

        water_fsm_output = Fsm_Update(&fsm_input);
        Apply_Fsm_Output(&water_fsm_output);
        OLED_ShowLevel(filter_valid ? water_level_percent : -1.0f);
        OLED_ShowStatus(Fsm_GetStateName(water_fsm_output.state));
#endif

        Logger_Printf(
            "WATER | raw=%6.2f cm | filtered=%6.2f cm | level=%6.2f%% | "
            "state=%s | pump=%u buzzer=%u | error=%s | echo=%lu us | "
            "valid=%u ready=%u count=%u reset=%u\r\n",
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
            (unsigned int)Filter_GetSampleCount(&water_level_filter),
            user_reset ? 1U : 0U
        );

        ultrasonic_last_reported_status = ULTRASONIC_STATUS_DONE;
        distance_processed = true;
    }

    status = Ultrasonic_GetStatus();

    /* A timeout/error is treated as a sensor failure.  Do this once per
     * error event to avoid flooding UART and repeatedly rendering OLED. */
    if (((status == ULTRASONIC_STATUS_TIMEOUT) ||
        (status == ULTRASONIC_STATUS_OUT_OF_RANGE) ||
         (status == ULTRASONIC_STATUS_ERROR)) &&
        (status != ultrasonic_last_reported_status))
    {
        FsmInput_t fsm_input = {
            water_level_percent,
            false,
            user_reset,
            now_ms
        };

        water_fsm_output = Fsm_Update(&fsm_input);
        Apply_Fsm_Output(&water_fsm_output);

        OLED_ShowLevel(-1.0f);
        OLED_ShowStatus(Fsm_GetStateName(water_fsm_output.state));

        Logger_Printf(
            "SENSOR ERROR | status=%s | state=%s | pump=%u buzzer=%u | error=%s | reset=%u\r\n",
            Ultrasonic_GetStatusName(status),
            Fsm_GetStateName(water_fsm_output.state),
            water_fsm_output.pump_on ? 1U : 0U,
            water_fsm_output.buzzer_on ? 1U : 0U,
            Fsm_GetErrorName(water_fsm_output.error),
            user_reset ? 1U : 0U
        );

        ultrasonic_last_reported_status = status;
    }

    /* If the reset button is pressed while there is no new ultrasonic sample,
     * still pass the request to the FSM. */
    if (user_reset &&
        !distance_processed &&
        (status != ULTRASONIC_STATUS_TIMEOUT) &&
        (status != ULTRASONIC_STATUS_OUT_OF_RANGE) &&
        (status != ULTRASONIC_STATUS_ERROR))
    {
        FsmInput_t fsm_input = {
            water_level_percent,
            Filter_IsReady(&water_level_filter),
            true,
            now_ms
        };

        water_fsm_output = Fsm_Update(&fsm_input);
        Apply_Fsm_Output(&water_fsm_output);
        OLED_ShowStatus(Fsm_GetStateName(water_fsm_output.state));

        Logger_Printf(
            "FSM RESET | state=%s | pump=%u buzzer=%u | error=%s\r\n",
            Fsm_GetStateName(water_fsm_output.state),
            water_fsm_output.pump_on ? 1U : 0U,
            water_fsm_output.buzzer_on ? 1U : 0U,
            Fsm_GetErrorName(water_fsm_output.error)
        );
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

static void Apply_Fsm_Output(const FsmOutput_t *output)
{
    if (output == NULL)
    {
        Relay_Off();
        Buzzer_Off();
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

/**
  * @brief GPIO EXTI callback. PB5 is the user-reset button.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_5)
    {
        const uint32_t now_ms = HAL_GetTick();

        /* Simple software debounce: ignore edges within 150 ms. */
        if ((uint32_t)(now_ms - fsm_last_reset_button_ms) >= 150U)
        {
            fsm_reset_request = true;
            fsm_last_reset_button_ms = now_ms;
        }
    }
}

#if RUN_STARTUP_SELF_TESTS
static float Test_AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void Filter_Test_Run(void)
{
    /* Keep startup tests available for manual verification. */
    DistanceFilter_t filter;
    float filtered_cm = 0.0f;
    float water_level = 0.0f;
    const float samples[] = {20.1f, 20.3f, 19.8f, 20.7f, 20.0f};

    Filter_Init(&filter);
    Filter_SetTankHeightCm(&filter, WATER_TANK_HEIGHT_CM);

    for (uint8_t i = 0U; i < (uint8_t)(sizeof(samples) / sizeof(samples[0])); ++i)
    {
        (void)Filter_UpdateWaterLevel(
            &filter,
            samples[i],
            &filtered_cm,
            &water_level
        );
    }

    Logger_Printf(
        "FILTER SELF TEST | filtered=%6.2f cm | level=%6.2f%% | ready=%u\r\n",
        filtered_cm,
        water_level,
        Filter_IsReady(&filter) ? 1U : 0U
    );

    (void)Test_AbsFloat(0.0f);
}

static void FSM_Test_Run(void)
{
    const FsmInput_t tests[] = {
        {50.0f, true, false, 0U},
        {15.0f, true, false, 1000U},
        {90.0f, true, false, 2000U},
        {99.0f, true, false, 3000U},
        {95.0f, true, false, 4000U},
        {50.0f, false, false, 5000U},
        {50.0f, true, true, 6000U}
    };

    Fsm_Init();

    Logger_Print("FSM SELF TEST\r\n");
    for (uint8_t i = 0U; i < (uint8_t)(sizeof(tests) / sizeof(tests[0])); ++i)
    {
        FsmOutput_t output = Fsm_Update(&tests[i]);
        Logger_Printf(
            "T%u | state=%s | pump=%u | buzzer=%u | error=%s\r\n",
            (unsigned int)(i + 1U),
            Fsm_GetStateName(output.state),
            output.pump_on ? 1U : 0U,
            output.buzzer_on ? 1U : 0U,
            Fsm_GetErrorName(output.error)
        );
    }
}
#endif

/* USER CODE END 4 */

void Error_Handler(void)
{
    __disable_irq();

    Relay_Off();
    Buzzer_Off();

    while (1)
    {
        /* Keep outputs in a safe state. */
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif /* USE_FULL_ASSERT */
