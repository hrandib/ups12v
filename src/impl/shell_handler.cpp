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
#include "cal_data.h"
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

static uint32_t convertVoltage2Percents(uint32_t val, const bat_lut_t& lut)
{
    using namespace std;
    if(val <= lut.front().first) {
        return 0;
    }
    if(val >= lut.back().first) {
        return 100;
    }

    const auto result = ranges::find_if(lut, [val](auto entry) { return entry.first > val; });
    const auto [v2, p2] = *result;
    const auto prev = result - 1;
    const auto [v1, p1] = *prev;
    auto percent_offset = 10 * (val - v1) * (p2 - p1) / (v2 - v1);
    return p1 + (percent_offset + 5) / 10;
}

static uint32_t convertPercents2Voltage(uint32_t val, const bat_lut_t& lut)
{
    using namespace std;
    auto result = ranges::find_if(lut, [val](auto entry) { return entry.second > val; });
    const auto [v2, p2] = *result;
    const auto prev = result - 1;
    const auto [v1, p1] = *prev;
    auto volt_offset = 10 * (val - p1) * (v2 - v1) / (p2 - p1);
    return v1 + (volt_offset + 5) / 10;
}

void cmd_poll(BaseSequentialStream* chp, int argc, char* /*argv*/[])
{
    if(!argc) {
        using namespace monitor;
        using enum State;
        auto* asyncCh = (BaseAsynchronousChannel*)chp;
        while(true) {
            uint16_t vBat = voltages[AdcVBat].load(std::memory_order_relaxed);
            auto vBal = vBat - (voltages[AdcBat1].load(std::memory_order_relaxed) * 2);
            auto percents = convertVoltage2Percents(vBat, (state == Discharge) ? DISCHARGE_LUT : CHARGE_LUT);
            chprintf(
              chp, "%u  %u  %d  %u  %s\r\n", (uint16_t)voltages[AdcMain], vBat, vBal, percents, toString(state).data());
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
                   "  in mV, battery level % and the current state\r\n  Press CTRL-C to exit");
    }
}

static void cutoff(monitor::State what,
                   std::atomic_uint16_t& cutoffVal,
                   BaseSequentialStream* chp,
                   int argc,
                   char* argv[])
{
    using enum monitor::State;
    constexpr auto minPercent = 50;
    constexpr auto maxPercent = 100;
    const auto& lut = (what == Charge ? CHARGE_LUT : DISCHARGE_LUT);
    do {
        if(argc == 1) {
            uint32_t val = atoi(argv[0]);
            if(minPercent <= val && val <= maxPercent) {
                val = convertPercents2Voltage(val, lut);
            }
            else {
                chprintf(chp, "The value is not in valid range\r\n");
                break;
            }
            chprintf(chp, "Per element: %umV, Battery: %umV\r\n", val / 2, val);
            cutoffVal = val;
            return;
        }
    } while(false);
    chprintf(chp, "Limits %s %s level\r\n", what == Discharge ? "IDLE" : "", toString(what));
    shellUsage(chp,
               "Set cut-off battery level in percents.\r\n"
               "  The input value must be in the range 50-100\r\n");
}

static void cmd_cutoff_charge(BaseSequentialStream* chp, int argc, char* argv[])
{
    cutoff(monitor::State::Charge, monitor::chargeCutoff, chp, argc, argv);
}

static void cmd_cutoff_discharge(BaseSequentialStream* chp, int argc, char* argv[])
{
    cutoff(monitor::State::Discharge, monitor::idleDischargeCutoff, chp, argc, argv);
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
