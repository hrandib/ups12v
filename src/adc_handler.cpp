/*
 * Copyright (c) 2022 Dmytro Shestakov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "adc_handler.h"

#include "ch.h"
#include "hal.h"

#define ADC_GRP_NUM_CHANNELS 5
#define ADC_GRP_BUF_DEPTH 8

// 85% battery charge by default;
std::atomic_uint16_t Analog::cutoff_voltage = 4100;

static adcsample_t samples[ADC_GRP_NUM_CHANNELS * ADC_GRP_BUF_DEPTH];

/*
 * ADC streaming callback.
 */
static size_t nx, ny = 0;
static void adccallback(ADCDriver* adcp)
{
    if(adcIsBufferComplete(adcp)) {
        nx += 1;
    }
    else {
        ny += 1;
    }
}

static void adcerrorcallback(ADCDriver* adcp, adcerror_t err)
{
    (void)adcp;
    (void)err;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 16 samples of 4 channel, SW triggered.
 * Channels:    IN10.
 */
static const ADCConversionGroup adcgrpcfg = {
  FALSE,
  ADC_GRP_NUM_CHANNELS,
  NULL,
  adcerrorcallback,
  ADC_CFGR1_CONT | ADC_CFGR1_RES_12BIT,                                                              /* CFGR1 */
  ADC_TR(0, 0),                                                                                      /* TR */
  ADC_SMPR_SMP_239P5,                                                                                /* SMPR */
  ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 | ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL17 /* CHSELR */
};

void runAdc()
{
    /*
     * Activates the ADC1 driver and the temperature sensor.
     */
    adcStart(&ADCD1, NULL);
    adcSTM32SetCCR(ADC_CCR_VREFEN);

    /*
     * Linear conversion.
     */
    //    adcConvert(&ADCD1, &adcgrpcfg1, samples, ADC_GRP1_BUF_DEPTH);
    //    chThdSleepMilliseconds(1000);

    /*
     * Starts an ADC continuous conversion.
     */
    adcStartConversion(&ADCD1, &adcgrpcfg, samples, ADC_GRP_BUF_DEPTH);
}
