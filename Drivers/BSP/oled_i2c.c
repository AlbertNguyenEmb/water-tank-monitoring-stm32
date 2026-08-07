#include "oled_i2c.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool oled_initialized = false;

static float oled_level = -1.0f;
static char oled_status[16] = "INIT";

static void OLED_FormatLevel(
    char *buffer,
    uint16_t buffer_size,
    float level
)
{
    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if (level < 0.0f)
    {
        snprintf(
            buffer,
            buffer_size,
            "LEVEL: ERROR"
        );

        return;
    }

    if (level > 100.0f)
    {
        level = 100.0f;
    }

    uint16_t scaled =
        (uint16_t)(level * 10.0f + 0.5f);

    uint16_t integer_part = scaled / 10U;
    uint16_t decimal_part = scaled % 10U;

    snprintf(
        buffer,
        buffer_size,
        "LEVEL:%3u.%1u%%",
        integer_part,
        decimal_part
    );
}

static void OLED_Render(void)
{
    char level_line[20];
    char status_line[20];

    if (!oled_initialized)
    {
        return;
    }

    OLED_FormatLevel(
        level_line,
        sizeof(level_line),
        oled_level
    );

    
     //Tránh tràn bộ đệm.
     
    snprintf(
        status_line,
        sizeof(status_line),
        "ST:%-15.15s",
        oled_status
    );

    //Xóa framebuffer rồi vẽ lại toàn bộ nội dung.
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(
        "WATER MONITOR",
        Font_7x10,
        White
    );

    ssd1306_SetCursor(0, 11);
    ssd1306_WriteString(
        level_line,
        Font_7x10,
        White
    );

    ssd1306_SetCursor(0, 22);
    ssd1306_WriteString(
        status_line,
        Font_7x10,
        White
    );

    ssd1306_UpdateScreen();
}

void OLED_Init(void)
{
    ssd1306_Init();

    oled_level = -1.0f;

    strncpy(
        oled_status,
        "INIT",
        sizeof(oled_status) - 1U
    );

    oled_status[sizeof(oled_status) - 1U] = '\0';

    oled_initialized = true;

    OLED_Render();
}

void OLED_ShowLevel(float level)
{
    oled_level = level;
    OLED_Render();
}

void OLED_ShowStatus(const char *st)
{
    if (st == NULL)
    {
        st = "UNKNOWN";
    }

    strncpy(
        oled_status,
        st,
        sizeof(oled_status) - 1U
    );

    oled_status[sizeof(oled_status) - 1U] = '\0';

    OLED_Render();
}