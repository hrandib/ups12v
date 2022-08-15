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
#include "monitor.h"
#include "usbcfg.h"
#include <cstdlib>

static void cmd_poll(BaseSequentialStream* chp, int argc, char* argv[]);
static void cmd_cutoff_charge(BaseSequentialStream* chp, int argc, char* argv[]);
static void cmd_cutoff_discharge(BaseSequentialStream* chp, int argc, char* argv[]);

static const ShellCommand commands[] = {{"poll", cmd_poll},
                                        {"limit-charge", cmd_cutoff_charge},
                                        {"limit-discharge", cmd_cutoff_discharge},
                                        {nullptr, nullptr}};
static char histbuf[128];
static const ShellConfig shell_cfg = {(BaseSequentialStream*)&SDU1, commands, histbuf, 128};

// Continuously report: main(Output/Input), Bat1, Bat2, USB VBUS voltages in mV
void cmd_poll(BaseSequentialStream* chp, int argc, char* /*argv*/[])
{
    if(!argc) {
        using namespace monitor;
        auto* asyncCh = (BaseAsynchronousChannel*)chp;
        while(chnGetTimeout(asyncCh, TIME_IMMEDIATE) == MSG_TIMEOUT) {
            chprintf(chp,
                     "%u  %u  %u  %u\r\n",
                     (uint16_t)voltages[0],
                     (uint16_t)voltages[1],
                     (uint16_t)voltages[2],
                     (uint16_t)voltages[3]);
            chThdSleepSeconds(1);
        }
    }
    else {
        shellUsage(chp, "Continuously reports Main(Output/Input), BAT1, BAT2, USB_VBUS voltages in mV");
    }
}

static inline uint32_t convertPercents2Voltage(uint32_t val)
{
    //                              50%   55%   60%   65%   70%   75%   80%   85%   90%   95%   100%
    static constexpr uint16_t lut[]{3840, 3850, 3870, 3910, 3950, 3980, 4020, 4080, 4110, 4150, 4200};
    return lut[(val - 50) / 5];
}

static void cutoff(const char* what, std::atomic_uint16_t& cutoffVal, BaseSequentialStream* chp, int argc, char* argv[])
{
    do {
        if(argc == 1) {
            uint32_t val = atoi(argv[0]);
            if(50 <= val && val <= 100) {
                val = convertPercents2Voltage(val);
            }
            else if(val <= 3800 || 4200 <= val) {
                chprintf(chp, "The value is not in valid range\r\n");
                break;
            }
            chprintf(chp, "%u mV\r\n", val);
            cutoffVal = val;
            return;
        }
    } while(false);
    chprintf(chp, "Limits %s level\r\n", what);
    shellUsage(chp,
               "Set cut-off voltage or percentage.\r\n"
               "  Set cut-off voltage if input value in the range 3500-4200\r\n"
               "  or max charge percentage if input value in the range 50-100\r\n");
}

static void cmd_cutoff_charge(BaseSequentialStream* chp, int argc, char* argv[])
{
    cutoff("charge", monitor::chargeCutoff, chp, argc, argv);
}

static void cmd_cutoff_discharge(BaseSequentialStream* chp, int argc, char* argv[])
{
    cutoff("idle discharge", monitor::idleDischargeCutoff, chp, argc, argv);
}

static THD_WORKING_AREA(SHELL_WA_SIZE, 512);
void shellRun()
{
    shellInit();
    auto* thd = chThdCreateStatic(SHELL_WA_SIZE, sizeof(SHELL_WA_SIZE), NORMALPRIO, shellThread, (void*)&shell_cfg);
    chRegSetThreadNameX(thd, "Shell");
}
