/**
 * @file Final_Project.c
 * @author "YOUR NAME HERE"
 * @brief 
 * @version 0.1 (YOU MAY UPDATE THIS)
 * @date FILL IN DATE
 */

#include "Active_Lab.h"
#include "main.h"
#include "Functions.h"

#if defined(project)



char APP_DESCRIPTION[] = "BME 463: Final Project - YOUR NAME";

// ----- PIN ASSIGNMENT DEFINITIONS -----

// Pin for PICO on board LED
#define LED_PIN PICO_DEFAULT_LED_PIN 

// Pin for interrupt flag
#define FLAG_PIN  22   

// ADC Pin and Channel
#define ADC_GPIO      26
#define ADC_CH        0

// ---- Timing and Timers ----
#define SAMPLE_PERIOD_US   5000 // Sampling Frequency -> 200 Hz
static repeating_timer_t samp_timer;
static volatile bool sample_flag = false;

static bool samp_timer_cb(repeating_timer_t *t)
{
    sample_flag = true;
    gpio_put(FLAG_PIN, 1);
    return true;
}

// ---------------------------------------------------------------------------------
// Implement values for filter coefficients ----------------------------
// ---------------------------------------------------------------------------------


// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------


// Initialize additonal HW pins you wish to use here if needed
void app_init_hw(void){
    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Initialize interrupt flag pin
    gpio_init(FLAG_PIN);
    gpio_set_dir(FLAG_PIN, GPIO_OUT);
    gpio_put(FLAG_PIN, 0);

    // Initialize DAC
    MCP4728_init(i2c1, 14, 15, 400 * 1000, 0x60);
    
    // ADC init
    adc_init();
    adc_gpio_init(ADC_GPIO); 
    adc_select_input(ADC_CH);

    serial_start_boot_banner(APP_DESCRIPTION, NAME);

    // Start periodic sampling timer
    add_repeating_timer_us(
        -SAMPLE_PERIOD_US,
        samp_timer_cb,
        NULL,
        &samp_timer
    );
}

// You may implement your Final Project here
void app_main(void){
    while (true) {


    if (!sample_flag) {
        tight_loop_contents();
        continue;
    }
    sample_flag = false;

    uint16_t raw_adc_value = adc_read();
    float new_input_sample = (float)raw_adc_value / 4095.0f;
    detect_afib(new_input_sample);
    }
}
#endif