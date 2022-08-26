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

#include "ch.h"
#include "monitor.h"
#include "ssd1306.h"
#include <cstdio>
#include <cstdlib>

namespace display {

using namespace Mcucpp;

using Scl = Pa6;
using Sda = Pa7;
using Twi = i2c::SoftTwi<Scl, Sda>;
using Disp = ssd1306<Twi, ssd1306_128x32>;

static std::pair<uint16_t, uint16_t> mv2v(uint16_t val)
{
    auto result = div(val, 1000);
    auto decimals = (result.rem + 5) / 10;
    auto integer = result.quot;
    if(decimals == 100) {
        ++integer;
        decimals = 0;
    }
    return {integer, decimals};
}

static void displayStatus()
{
    char buf[40];
    using namespace monitor;
    //    if(state != State::Idle) {
    Disp::SetXY(0, 0);
    sprintf(buf, "%-10s", toString(state));
    Disp::Puts2X(buf);
    Disp::SetXY(0, 2);
    const char* labelVMain = state == State::Discharge ? "Output" : "Input ";
    sprintf(buf, "%s  VBat  Balance", labelVMain);
    Disp::Puts(buf);
    Disp::SetXY(0, 3);

    auto vBat = voltages[AdcVBat].load(std::memory_order_relaxed);
    auto vMain = voltages[AdcMain].load(std::memory_order_relaxed);
    auto vBal = vBat - (voltages[AdcBat1].load(std::memory_order_relaxed) * 2);
    auto vBatFixed = mv2v(vBat);
    auto vMainFixed = mv2v(vMain);
    sprintf(
      buf, "%u.%02uV  %u.%02uV  %dmV ", vMainFixed.first, vMainFixed.second, vBatFixed.first, vBatFixed.second, vBal);
    Disp::Puts(buf);
    //    }
}

static THD_WORKING_AREA(DISP_WA_SIZE, 512);
THD_FUNCTION(displayThread, )
{
    chThdSetPriority(HIGHPRIO - 1);
    Twi::Init();
    chThdSleepMilliseconds(100);
    Disp::Init();
    Disp::Fill();
    Disp::SetContrast(10);
    Disp::Puts2X("  12V UPS");
    chThdSleepSeconds(1);
    Disp::Fill();
    while(true) {
        displayStatus();
        chThdSleepSeconds(2);
    }
}

void run()
{
    auto* thd = chThdCreateStatic(DISP_WA_SIZE, sizeof(DISP_WA_SIZE), NORMALPRIO, displayThread, nullptr);
    chRegSetThreadNameX(thd, "display");
}

} // display
