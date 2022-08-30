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
#include <ranges>

static void cmd_poll(BaseSequentialStream* chp, int argc, char* argv[]);
static void cmd_cutoff_charge(BaseSequentialStream* chp, int argc, char* argv[]);
static void cmd_cutoff_discharge(BaseSequentialStream* chp, int argc, char* argv[]);
static void print_cutoff(BaseSequentialStream* chp, int argc, char* argv[]);

static const ShellCommand commands[] = {{"poll", cmd_poll},
                                        {"limit-charge", cmd_cutoff_charge},
                                        {"limit-discharge", cmd_cutoff_discharge},
                                        {"limits", print_cutoff},
                                        {nullptr, nullptr}};
static char histbuf[128];
static const ShellConfig shell_cfg = {(BaseSequentialStream*)&SDU1, commands, histbuf, 128};
constexpr char CTRL_C = 0x03;

static inline uint32_t convertDischargeVoltage2Percents(uint32_t val)
{
    using namespace std;
    static constexpr array<pair<uint16_t, uint16_t>, 18> lut{{{6250, 0},
                                                              {6750, 6},
                                                              {6970, 12},
                                                              {7100, 18},
                                                              {7210, 24},
                                                              {7280, 30},
                                                              {7330, 36},
                                                              {7380, 42},
                                                              {7420, 48},
                                                              {7460, 54},
                                                              {7510, 60},
                                                              {7560, 66},
                                                              {7630, 72},
                                                              {7690, 78},
                                                              {7750, 84},
                                                              {7810, 90},
                                                              {7870, 96},
                                                              {7910, 100}}};
    if(val <= lut.front().first) {
        return 0;
    }
    if(val >= lut.back().first) {
        return 100;
    }

    auto result = ranges::find_if(lut, [val](auto entry) { return entry.first > val; });
    auto prev = result - 1;
    auto v2 = result->first;
    auto p2 = result->second;
    auto v1 = prev->first;
    auto p1 = prev->second;
    auto percent = 10 * (val - v1) * (p2 - p1) / (v2 - v1);
    return p1 + (percent + 5) / 10;
}

void cmd_poll(BaseSequentialStream* chp, int argc, char* /*argv*/[])
{
    if(!argc) {
        using namespace monitor;
        auto* asyncCh = (BaseAsynchronousChannel*)chp;
        while(true) {
            uint16_t vBat = voltages[AdcVBat].load(std::memory_order_relaxed);
            auto vBal = vBat - (voltages[AdcBat1].load(std::memory_order_relaxed) * 2);
            if(state == State::Discharge) {
                auto dischargePercents = convertDischargeVoltage2Percents(vBat);
                chprintf(chp,
                         "%u  %u  %d  %u%% %s\r\n",
                         (uint16_t)voltages[AdcMain],
                         vBat,
                         vBal,
                         dischargePercents,
                         toString(state).data());
            }
            else {
                chprintf(chp, "%u  %u  %d  %s\r\n", (uint16_t)voltages[AdcMain], vBat, vBal, toString(state).data());
            }
            if(auto msg = chnGetTimeout(asyncCh, TIME_S2I(1)); msg == CTRL_C) {
                break;
            }
            else if(msg != MSG_TIMEOUT) {
                chThdSleepSeconds(1);
            }
        }
    }
    else {
        shellUsage(chp,
                   "Continuously reports Main(Output/Input), VBAT, BAT2-BAT1 difference voltages\r\n"
                   "  in mV and the current state\r\n  Press CTRL-C to exit");
    }
}

static inline uint32_t convertChargePercents2Voltage(uint32_t val)
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
                val = convertChargePercents2Voltage(val);
            }
            else if(val < 3800 || 4200 < val) {
                chprintf(chp, "The value is not in valid range\r\n");
                break;
            }
            chprintf(chp, "Per element: %umV, Battery: %umV\r\n", val, val * 2);
            cutoffVal = val * 2;
            return;
        }
    } while(false);
    chprintf(chp, "Limits %s level\r\n", what);
    shellUsage(chp,
               "Set cut-off voltage or percentage.\r\n"
               "  Set cut-off voltage if input value in the range 3800-4200\r\n"
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

static void print_cutoff(BaseSequentialStream* chp, int /*argc*/, char* /*argv*/[])
{
    chprintf(chp,
             "Charge will be stopped reaching %umV and started from %umV\r\n",
             monitor::chargeCutoff.load(),
             monitor::idleDischargeCutoff.load());
}

static THD_WORKING_AREA(SHELL_WA_SIZE, 512);
void shellRun()
{
    shellInit();
    auto* thd = chThdCreateStatic(SHELL_WA_SIZE, sizeof(SHELL_WA_SIZE), NORMALPRIO, shellThread, (void*)&shell_cfg);
    chRegSetThreadNameX(thd, "shell");
}
