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
#include <array>
#include <numeric>

#define ADC_GRP_BUF_DEPTH 16
#define ADC_GRP_CHANNELS 5
#define ADC_VREF_CHANNEL 4

using buf_t = adcsample_t[ADC_GRP_BUF_DEPTH][ADC_GRP_CHANNELS];

static buf_t samples;

static void adcerrorcallback(ADCDriver* adcp, adcerror_t err)
{
    (void)adcp;
    (void)err;
}

static const ADCConversionGroup adcgrpcfg = {
  FALSE,
  ADC_GRP_CHANNELS,
  nullptr,
  adcerrorcallback,
  ADC_CFGR1_CONT | ADC_CFGR1_RES_12BIT,                                                              /* CFGR1 */
  ADC_TR(0, 0),                                                                                      /* TR */
  ADC_SMPR_SMP_71P5,                                                                                 /* SMPR */
  ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 | ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL17 /* CHSELR */
};

void initAdc()
{
    adcStart(&ADCD1, nullptr);
    adcSTM32SetCCR(ADC_CCR_VREFEN);
}

static uint16_t sum(const adcsample_t* samples)
{
    using namespace std;
    uint16_t result{};
    for(size_t i{}; i < ADC_GRP_BUF_DEPTH; ++i) {
        result += *(samples + (i * ADC_GRP_CHANNELS));
    }
    return result;
}

static inline uint16_t getVdda(uint16_t vref)
{
    static const uint16_t& VREFINT_CAL = *(const uint16_t*)0x1FFFF7BA;
    const auto VDDA = (3300U * (VREFINT_CAL - 8) * ADC_GRP_BUF_DEPTH) / vref;
    return VDDA;
}

constexpr uint32_t CAL_DATA[monitor::AdcChNumber] = {1, 1, 1, 1};
constexpr uint32_t FULL_SCALE = 4095U;

msg_t getVoltages(monitor::adc_data_t& voltages)
{
    using namespace monitor;
    msg_t result = adcConvert(&ADCD1, &adcgrpcfg, (adcsample_t*)samples, ADC_GRP_BUF_DEPTH);
    uint32_t avgBuf[ADC_GRP_CHANNELS];
    if(result == MSG_OK) {
        for(size_t i{}; i < ADC_GRP_CHANNELS; ++i) {
            avgBuf[i] = sum(&samples[0][i]);
        }
        uint32_t vdda = getVdda(avgBuf[ADC_VREF_CHANNEL]);
        for(size_t i{}; i < monitor::AdcChNumber; ++i) {
            uint16_t val = (avgBuf[i] * vdda) * CAL_DATA[i] / (FULL_SCALE * ADC_GRP_BUF_DEPTH);
            voltages[i] = val;
        }
    }
    return result;
}
