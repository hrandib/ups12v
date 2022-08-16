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

#include "monitor.h"
#include "ch.h"
#include "hal.h"
#include <array>
#include <numeric>

extern msg_t getVoltages(monitor::adc_data_t& voltages);

namespace monitor {

template<typename T>
class MovingAverageBuf
{
private:
    std::array<T, 8> buf_{};
    size_t i_{};
public:
    MovingAverageBuf& add(T val)
    {
        buf_[i_++] = val;
        if(buf_.size() == i_) {
            i_ = 0;
        }
        return *this;
    }
    uint16_t getMean()
    {
        return std::accumulate(buf_.cbegin(), buf_.cend(), 0) / buf_.size();
    }
};

// 85% battery charge by default
atomic_uint16_t chargeCutoff = 4100U * 2;
// 55% battery charge by default
atomic_uint16_t idleDischargeCutoff = 3850U * 2;

std::atomic<State> state;
adc_data_t voltages;

const char* stateString[] = {"Idle", "Trickle", "Discharge", "Charge"};

constexpr uint16_t SWITCH_12V_THRESHOLD = 11900U;
constexpr uint16_t TRICKLE_HYST = 100U;

std::array<MovingAverageBuf<uint16_t>, AdcChNumber> maArray;

static THD_WORKING_AREA(SHELL_WA_SIZE, 128);
THD_FUNCTION(monitorThread, )
{
    using enum AdcChannels;
    while(true) {
        adc_data_t temp_voltages;
        getVoltages(temp_voltages);
        for(size_t i{}; i < AdcChNumber; ++i) {
            maArray[i].add(temp_voltages[i]);
            voltages[i] = maArray[i].getMean();
        }

        uint16_t batVoltage = voltages[AdcVBat];
        switch(state) {
            using enum State;
            case Idle:
                if(voltages[AdcMain] < SWITCH_12V_THRESHOLD) {
                    state = Discharge;
                }
                else if(batVoltage < idleDischargeCutoff) {
                    state = Charge;
                    palSetLine(LINE_CHRG_EN);
                }
                else if(batVoltage < (chargeCutoff - TRICKLE_HYST)) {
                    state = Trickle;
                    palSetLine(LINE_TRICKLE_EN);
                }
                break;
            case Trickle:
                if(voltages[AdcMain] < SWITCH_12V_THRESHOLD) {
                    state = Discharge;
                    palClearLine(LINE_TRICKLE_EN);
                }
                else if(batVoltage < idleDischargeCutoff) {
                    state = Charge;
                    palClearLine(LINE_TRICKLE_EN);
                    palSetLine(LINE_CHRG_EN);
                }
                else if(batVoltage > chargeCutoff) {
                    state = Idle;
                    palClearLine(LINE_TRICKLE_EN);
                };
                break;
            case Discharge:
                if(voltages[AdcMain] > SWITCH_12V_THRESHOLD) {
                    state = Idle;
                }
                break;
            case Charge:
                if(voltages[AdcMain] < SWITCH_12V_THRESHOLD) {
                    state = Discharge;
                    palClearLine(LINE_CHRG_EN);
                }
                else if(batVoltage > chargeCutoff) {
                    palClearLine(LINE_CHRG_EN);
                    state = Idle;
                };
                break;
        }
        chThdSleepMilliseconds(200);
    }
}

void run()
{
    auto* thd = chThdCreateStatic(SHELL_WA_SIZE, sizeof(SHELL_WA_SIZE), NORMALPRIO + 1, monitorThread, nullptr);
    chRegSetThreadNameX(thd, "monitor");
}

} // data
