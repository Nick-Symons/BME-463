#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "adc_ecg.h"

void adc_ecg_init(void)
{
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
}

float adc_ecg_read_voltage(void)
{
    uint16_t raw = adc_read();
    return raw * 3.3f / 4095.0f;
}