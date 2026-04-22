#include <stdio.h>
#include "pico/stdlib.h"

#include "adc_ecg.h"
#include "filters.h"
#include "qrs.h"
#include "features.h"
#include "classify.h"

#define FS 250
#define BUF 2500

float ecg[BUF];
int idx=0;

int main()
{
    stdio_init_all();
    adc_ecg_init();

    sleep_ms(2000);

    while(1)
    {
        float x = adc_ecg_read_voltage();
        float y = ecg_filter(x);

        ecg[idx]=y;

        qrs_update(y, idx);

        idx++;
        if(idx>=BUF) idx=0;

        if(idx % FS == 0)
        {
            float rrstd = calc_rr_std(qrs_peaks(),
                                      qrs_peak_count(),
                                      FS);

            float noise = calc_noise(ecg, BUF);

            RhythmClass r =
                classify(noise, rrstd,
                         qrs_peak_count());

            printf("%s\n", class_name(r));
        }

        sleep_us(4000);
    }
}