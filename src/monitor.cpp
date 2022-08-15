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

extern msg_t getVoltages(monitor::adc_data_t& voltages);

namespace monitor {

// 85% battery charge by default
atomic_uint16_t chargeCutoff = 4100;
// 55% battery charge by default
atomic_uint16_t idleDischargeCutoff = 3850;

std::atomic<State> state;
adc_data_t voltages;

static THD_WORKING_AREA(SHELL_WA_SIZE, 128);
THD_FUNCTION(monitorThread, )
{
    while(true) {
        getVoltages(voltages);
        chThdSleepMilliseconds(200);
    }
}

void run()
{
    auto* thd = chThdCreateStatic(SHELL_WA_SIZE, sizeof(SHELL_WA_SIZE), NORMALPRIO + 1, monitorThread, nullptr);
    chRegSetThreadNameX(thd, "Monitor");
}

} // data
