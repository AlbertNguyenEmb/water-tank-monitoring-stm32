#include "fsm.h"
#include "filter.h"
#include "water_sensor.h"

static FSM_State_t s_state = FSM_STATE_INIT;

/**
 * @brief  Dung chung cho Init va cho luc Error phuc hoi: doc % hien tai
 *         va phan loai vao dung 1 trong 3 trang thai cu the.
 *         can < % < tran duoc gan vao FULL (mac dinh an toan: bom tat).
 */
static FSM_State_t Classify(float percent)
{
    if (percent >= TH_NGUONG_TRAN)
    {
        return FSM_STATE_OVERFLOW;
    }
    else if (percent <= TH_NGUONG_CAN)
    {
        return FSM_STATE_FILLING;
    }
    else
    {
        return FSM_STATE_FULL;
    }
}

void FSM_Init(void)
{
    s_state = FSM_STATE_INIT;
}

FSM_State_t FSM_GetState(void)
{
    return s_state;
}

void FSM_Run(void)
{
    /* FSM_Run() khong nhan tham so - tu doc cam bien va loc ben trong. */
    uint8_t adc_valid = WaterSensor_IsValid() ? 1U : 0U;
    float   percent;

    /* ADC bat thuong duoc kiem tra XUYEN SUOT, o BAT KY trang thai nao
     * (khong chi rieng Overflow) - moi chu ky FSM_Run() deu kiem tra lai. */
    if (!adc_valid)
    {
        s_state = FSM_STATE_ERROR;
        return;
    }

    /* Chi doc & loc khi ADC hop le */
    percent = Filter_Update(WaterSensor_ReadPercent());

    switch (s_state)
    {
    case FSM_STATE_INIT:
        /* Init: doc ADC lan dau, phan loai vao dung 1 trang thai cu the */
        s_state = Classify(percent);
        break;

    case FSM_STATE_FILLING:
        /* Filling KHONG duoc nhay truc tiep sang Overflow.
         * Vi tran > day, muon toi tran thi phai qua day truoc: o day ta
         * chuyen sang FULL truoc; neu % da vuot tran luon thi chu ky
         * FSM_Run() ke tiep (dang o FULL) se tiep tuc phat hien va
         * chuyen sang OVERFLOW - cham nhat 1 chu ky Timer (1-2s),
         * van dam bao khong "nhay tat" qua Full. */
        if (percent >= TH_NGUONG_DAY)
        {
            s_state = FSM_STATE_FULL;
        }
        break;

    case FSM_STATE_FULL:
        if (percent >= TH_NGUONG_TRAN)
        {
            s_state = FSM_STATE_OVERFLOW;      /* duong DUY NHAT vao Overflow */
        }
        else if (percent <= TH_NGUONG_CAN)
        {
            s_state = FSM_STATE_FILLING;
        }
        /* con lai (can < % < tran): giu nguyen FULL - vung chet hysteresis */
        break;

    case FSM_STATE_OVERFLOW:
        if (percent < TH_NGUONG_TRAN)
        {
            s_state = FSM_STATE_FULL;          /* thoat Overflow -> FULL, KHONG ve Init */
        }
        break;

    case FSM_STATE_ERROR:
        /* ADC hop le tro lai: doc lai % va phan loai lai, giong Init */
        s_state = Classify(percent);
        break;

    default:
        s_state = FSM_STATE_INIT;
        break;
    }
}