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

// clang-format off
#include "hal.h"
#include "chprintf.h"
#include "shell.h"
// clang-format on

#include "shell_handler.h"
#include "adc_handler.h"
#include "string_utils.h"
#include "usbcfg.h"
#include <cstdlib>

using namespace std::literals;

static THD_WORKING_AREA(SHELL_WA_SIZE, 512);
static void cmd_poll(BaseSequentialStream* chp, int argc, char* argv[]);
static void cmd_cutoff(BaseSequentialStream* chp, int argc, char* argv[]);

static const ShellCommand commands[] = {{"poll", cmd_poll}, {"cutoff", cmd_cutoff}, {nullptr, nullptr}};
static char histbuf[128];
static const ShellConfig shell_cfg = {(BaseSequentialStream*)&SDU1, commands, histbuf, 128};

// Continuously report: main(Output/Input), Bat1, Bat2, USB VBUS voltages in mV
void cmd_poll(BaseSequentialStream* chp, int /*argc*/, char* /*argv*/[])
{
    auto* asyncCh = (BaseAsynchronousChannel*)chp;
    while(chnGetTimeout(asyncCh, TIME_IMMEDIATE) == MSG_TIMEOUT) {
        chprintf(chp, "DUMMY\r\n");
        chThdSleepSeconds(1);
    }
}

static inline uint32_t convertPercents2Voltage(uint32_t val)
{
    //                              50%   55%   60%   65%   70%   75%   80%   85%   90%   95%   100%
    static constexpr uint16_t lut[]{3840, 3850, 3870, 3910, 3950, 3980, 4020, 4080, 4110, 4150, 4200};
    return lut[(val - 50) / 5];
}

void cmd_cutoff(BaseSequentialStream* chp, int argc, char* argv[])
{
    do {
        if(argc == 1) {
            auto val = atoi(argv[0]);
            if(50 <= val && val <= 100) {
                val = convertPercents2Voltage(val);
            }
            else if(val <= 3800 || 4200 <= val) {
                chprintf(chp, "The value is not in valid range\r\n");
                break;
            }
            chprintf(chp, "%u mV\r\n", val);
            Analog::cutoff_voltage = val;
            return;
        }
    } while(false);
    shellUsage(chp,
               "Set charge cut-off voltage or percentage.\r\n"
               "  Set cut-off voltage if input value in the range 3500-4200\r\n"
               "  or max charge percentage if input value in the range 50-100\r\n");
}

Shell::Shell()
{
    shellInit();
    auto* thd = chThdCreateStatic(SHELL_WA_SIZE, sizeof(SHELL_WA_SIZE), NORMALPRIO, shellThread, (void*)&shell_cfg);
    chRegSetThreadNameX(thd, "Shell");
}
