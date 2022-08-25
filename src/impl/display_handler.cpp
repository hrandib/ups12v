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

namespace display {

using namespace Mcucpp;

using Scl = Pa6;
using Sda = Pa7;
using Twi = i2c::SoftTwi<Scl, Sda>;
using Disp = ssd1306<Twi, ssd1306_128x32>;

static void displayStatus()
{
    using namespace monitor;
    for(size_t i{}; i < 60; i += 2) {
        Disp::Putch2X(0x31 + i);
        chThdSleepMilliseconds(500);
        Disp::Putch(0x32 + i);
        chThdSleepMilliseconds(500);
    }

    Disp::SetXY(32, 0);
    Disp::Puts2X(toString(state));
    Disp::SetXY(0, 2);
    Disp::Puts("Input  VBat   Diff");
    Disp::SetXY(0, 3);
    Disp::Puts("12.3V  8.21V  -9mV");
}

static THD_WORKING_AREA(DISP_WA_SIZE, 256);
THD_FUNCTION(displayThread, )
{
    chThdSetPriority(HIGHPRIO - 1);
    Twi::Init();
    chThdSleepMilliseconds(100);
    Disp::Init();
    Disp::Fill();
    Disp::SetContrast(10);
    //    Disp::Puts2X("12V UPS");
    //    chThdSleepSeconds(5);
    Disp::Fill();
    displayStatus();
    while(true) {
        chThdSleepSeconds(2);
    }
}

void run()
{
    auto* thd = chThdCreateStatic(DISP_WA_SIZE, sizeof(DISP_WA_SIZE), NORMALPRIO, displayThread, nullptr);
    chRegSetThreadNameX(thd, "display");
}

} // display
