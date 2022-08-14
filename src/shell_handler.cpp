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
#include "shell_handler.h"
#include "hal.h"
#include "chprintf.h"
#include "shell.h"
#include "string_utils.h"
#include "usbcfg.h"
// clang-format on

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

// Set charge cut-off voltage or percentage
// Set cut-off voltage if input value in the range 3500-4200
// or max charge percentage if input value <= 100
void cmd_cutoff(BaseSequentialStream* chp, int argc, char* argv[])
{
    auto* asyncCh = (BaseAsynchronousChannel*)chp;
    while(chnGetTimeout(asyncCh, TIME_IMMEDIATE) == MSG_TIMEOUT) {
        chprintf(chp, "DUMMY\r\n");
        chThdSleepSeconds(1);
    }
}

// void cmd_getdigital(BaseSequentialStream* chp, int argc, char** /*argv*/)
//{
//     if(!argc) {
//         chprintf(chp, "%x\r\n", Digital::input.GetBinaryVal());
//     }
//     else {
//         shellUsage(chp, "Get value of the digital input register in hex format");
//     }
// }

// void cmd_getcounters(BaseSequentialStream* chp, int argc, char* argv[])
//{
//     using namespace Digital;
//     static constexpr uint16_t AllChannelsMask = Utils::NumberToMask_v<Input::numChannels>;
//     uint16_t channelMask;
//     do {
//         if(argc == 0) {
//             channelMask = AllChannelsMask;
//         }
//         else if(argc == 1) {
//             auto mask = io::svtou(argv[0]);
//             if(mask && *mask > 0 && *mask <= AllChannelsMask) {
//                 channelMask = static_cast<uint16_t>(*mask);
//             }
//             else {
//                 break;
//             }
//         }
//         else {
//             break;
//         }
//         auto counters = input.GetCounters();
//         for(size_t i{}; i < counters.size(); ++i) {
//             if(channelMask & (1U << i)) {
//                 chprintf(chp, "%10u ", counters[i]);
//             }
//         }
//         chprintf(chp, "\r\n");
//         return;
//     } while(false);
//     shellUsage(chp,
//                "Get counters array"
//                "\r\nReturns selected counters from the array or all counters if no arguments passed"
//                "\r\n\tgetcounters [channel mask(0x01-0x3FF)]"
//                "\r\nExamples:"
//                "\r\nGet values of 2nd and 5th channels"
//                "\r\n\tgetcounters 0b10010"
//                "\r\n  or"
//                "\r\n\tgetcounters 0x12");
// }

// void cmd_setdigital(BaseSequentialStream* chp, int argc, char* argv[])
//{
//     using namespace Digital;
//     using Mode = OutputCommand::Mode;
//     static constexpr size_t valueMask = Utils::NumberToMask_v<OutputCommand::GetBusWidth()>;
//     OutputCommand cmd{};
//     do {
//         if(argc == 0) {
//             output.SendMessage(cmd);
//             chprintf(chp, "%x\r\n", cmd.GetValue());
//             return;
//         }
//         else if(argc == 3 && "set_clear"sv == argv[0]) {
//             auto setVal = io::svtou(argv[1]);
//             if(!setVal || *setVal > valueMask) {
//                 break;
//             }
//             auto clearVal = io::svtou(argv[2]);
//             if(!clearVal || *clearVal > valueMask) {
//                 break;
//             }
//             uint32_t val = *setVal | (*clearVal << OutputCommand::GetBusWidth());
//             cmd.Set(Mode::SetAndClear, val);
//             output.SendMessage(cmd);
//             chprintf(chp, "%x\r\n", cmd.GetValue());
//             return;
//         }
//         else if(argc == 2) {
//             if("set"sv == argv[0]) {
//                 cmd.SetMode(Mode::Set);
//             }
//             else if("clear"sv == argv[0]) {
//                 cmd.SetMode(Mode::Clear);
//             }
//             else if("write"sv == argv[0]) {
//                 cmd.SetMode(Mode::Write);
//             }
//             else if("toggle"sv == argv[0]) {
//                 cmd.SetMode(Mode::Toggle);
//             }
//             else {
//                 break;
//             }
//             auto value = io::svtou(argv[1]);
//             if(!value || *value > valueMask) {
//                 break;
//             }
//             cmd.SetValue((uint16_t)*value);
//             output.SendMessage(cmd);
//             chprintf(chp, "%x\r\n", cmd.GetValue());
//             return;
//         }
//     } while(false);
//     shellUsage(chp,
//                "Set state of the digital output register"
//                "\r\npossible modes: set, clear, set_clear, write, toggle"
//                "\r\nReturns modified register value"
//                // TODO: get mask at compile time
//                "\r\n\tsetdigital [mode] [mask(0-65535)]"
//                "\r\nExamples:"
//                "\r\n\tsetdigital set 0x2020"
//                "\r\n\tsetdigital set_clear 0x0088 0xFF00"
//                "\r\n\tsetdigital toggle 666");
// }

// void cmd_uptime(BaseSequentialStream* chp, int argc, char** /*argv[]*/)
//{
//     if(!argc) {
//         chprintf(chp, "%u\r\n", uptimeCounter.load());
//     }
// }

// void cmd_setmbid(BaseSequentialStream* chp, int argc, char* argv[])
//{
//     do {
//         if(!argc) {
//             chprintf(chp, "%u\r\n", modbus.GetID());
//         }
//         else if(argc == 1) {
//             auto id = io::svtou(argv[0]);
//             if(id && *id > 0 && *id < 247) {
//                 eMBErrorCode status = modbus.SetID((uint8_t)*id);
//                 if(status != MB_ENOERR) {
//                     chprintf(chp, "Error: %u\r\n", status);
//                 }
//             }
//             else {
//                 break;
//             }
//         }
//         else {
//             break;
//         }
//         return;
//     } while(false);
//     shellUsage(chp,
//                "Set MODBUS device ID"
//                "\r\nReturns current device ID if no arguments passed"
//                "\r\n\tsetmbid [1-246]");
// }

Shell::Shell()
{
    shellInit();
    auto* thd = chThdCreateStatic(SHELL_WA_SIZE, sizeof(SHELL_WA_SIZE), NORMALPRIO, shellThread, (void*)&shell_cfg);
    chRegSetThreadNameX(thd, "Shell");
}
