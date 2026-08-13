# FSM integration changes

The project is now prepared to run the real water-tank control loop instead of the ultrasonic-only demo mode.

## Main integration changes

- Disabled the ultrasonic UART demo path and made the main loop run the real water-level/FSM integration.
- Initialization order is now: GPIO -> I2C -> TIM2/TIM3 -> UART -> logger -> relay/buzzer -> OLED -> filter/FSM/ultrasonic.
- Added a complete sensor pipeline: HC-SR04 -> 5-sample moving-average filter -> water level percentage -> FSM -> relay/buzzer.
- Added filter warm-up handling. The FSM is not allowed to react until the 5-sample filter window is ready.
- Added PB5 EXTI reset handling with a 150 ms software debounce.
- Added safe output handling: relay and buzzer are forced OFF on startup and in Error_Handler().
- Added UART logs containing raw distance, filtered distance, level, FSM state, outputs, sensor status, and filter readiness.
- OLED now shows level and FSM state during the real application.
- Sensor timeout/error transitions into FSM sensor-error handling.

## FSM logic changes

- Added configuration range checks for all percentage thresholds and timeout.
- Added threshold-order validation so invalid configurations fall back to safe defaults.
- INIT now evaluates the first valid measurement directly instead of always spending one extra cycle in MONITORING.
- Sensor ERROR can only clear after a valid sensor measurement is available; a button press cannot mask an active sensor fault.
- Fill-timeout ERROR requires a user reset and a valid sensor measurement before returning to MONITORING.

## Files changed

- `Firmware/Core/Src/main.c`
- `Firmware/App/fsm_logic.c`
- `Firmware/Drivers/BSP/Oled.h` reference fixed in `main.c` for case-sensitive build environments.

The project still needs to be built and tested on the STM32 hardware because this environment does not have the ARM GCC toolchain installed.
