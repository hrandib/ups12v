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
#include "disp_stream.h"
#include "chprintf.h"
// clang-format on

#include "monitor.h"
#include "ssd1306.h"
#include "type_traits_ex.h"
#include <cstdio>
#include <cstdlib>

namespace display {

using namespace Mcucpp;

using Scl = Pa6;
using Sda = Pa7;
using Twi = i2c::SoftTwi<Scl, Sda>;
using Disp = ssd1306<Twi, ssd1306_128x32>;

static constexpr size_t FONT_WIDTH = 5;

// 1 pixel shift per ~127 secs
static constexpr size_t SHIFT_PERIOD_EXTENT = 7;
static constexpr auto CYCLE_MASK = Utils::NumberToMask_v<SHIFT_PERIOD_EXTENT>;

streams::DispStream<Disp> ds;

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

static inline uint8_t getStateLineLen(monitor::State st)
{
    static constexpr uint8_t CHAR_WIDTH = (FONT_WIDTH + 1) * 2; // char width + spacing for the double size text
    return monitor::toString(st).length() * CHAR_WIDTH;
}

static std::pair<uint8_t, uint8_t> getStateShift(monitor::State st)
{
    using enum monitor::State;
    static uint8_t cycle;
    static uint8_t shift;
    static auto prevState = Idle;
    static bool shiftReverse;
    if(st != prevState) {
        prevState = st;
        shift = 0;
        shiftReverse = false;
    }
    else if((++cycle & CYCLE_MASK) == CYCLE_MASK) {
        if(!shiftReverse) {
            ++shift;
            if(shift + getStateLineLen(st) > Disp::GetXRes()) {
                --shift;
                shiftReverse = true;
            }
        }
        else {
            --shift;
            if(shift == 0xFF) {
                ++shift;
                shiftReverse = false;
            }
        }
    }
    return {shift, shiftReverse ? 3 : 0};
}

constexpr char V12_LABEL_OUTPUT[] = "Output";
// length must be equal to the previous to preserve formatting
constexpr char V12_LABEL_INPUT[] = "Input ";
constexpr char BAT_BAL_LABELS[] = " VBat   Bal ";

static uint8_t getStaticTextShift()
{
    static constexpr uint8_t CHAR_WIDTH = FONT_WIDTH + 1; // char width + spacing
    static constexpr size_t TEXT_LINE_LEN = (sizeof(V12_LABEL_OUTPUT) - 1 + sizeof(BAT_BAL_LABELS) - 1) * CHAR_WIDTH;
    static uint8_t cycle;
    static uint8_t shift;
    static bool shiftReverse;
    if((++cycle & CYCLE_MASK) == CYCLE_MASK) {
        if(!shiftReverse) {
            ++shift;
            if(shift + TEXT_LINE_LEN > Disp::GetXRes()) {
                --shift;
                shiftReverse = true;
            }
        }
        else {
            --shift;
            if(shift == 0xFF) {
                ++shift;
                shiftReverse = false;
            }
        }
    }
    return shift;
}

static void displayStatus()
{
    using namespace monitor;
    using enum State;

    static uint8_t prevStateYpos;
    State st = state;
    auto [stateXpos, stateYpos] = getStateShift(st);
    // Clear possible tail artefacts during shifting
    if(stateXpos > 0) {
        Disp::Fill(stateXpos - 1, 1, stateYpos, 2);
    }
    if(stateYpos != prevStateYpos) {
        prevStateYpos = stateYpos;
        Disp::Fill();
    }
    auto valuesXpos = getStaticTextShift();
    auto valuesYpos = 0 + (!stateYpos ? 2 : 0);
    // Clear possible tail artefacts during shifting
    if(valuesXpos > 0) {
        Disp::Fill(valuesXpos - 1, 1, valuesYpos, 2);
    }
    Disp::SetXY(stateXpos, stateYpos);
    chprintf(ds.set2xFontSize(true).getBase(), "%s", toString(st).data());
    Disp::SetXY(valuesXpos, valuesYpos);
    const char* labelVMain = st == Discharge ? V12_LABEL_OUTPUT : V12_LABEL_INPUT;
    chprintf(ds.set2xFontSize(false).getBase(), "%s%s", labelVMain, BAT_BAL_LABELS);
    Disp::SetXY(valuesXpos, valuesYpos + 1);
    auto vBat = voltages[AdcVBat].load(std::memory_order_relaxed);
    auto vMain = voltages[AdcMain].load(std::memory_order_relaxed);
    auto vBal = abs(vBat - (voltages[AdcBat1].load(std::memory_order_relaxed) * 2));
    auto vBatFixed = mv2v(vBat);
    auto vMainFixed = mv2v(vMain);
    chprintf(ds.getBase(),
             "%u.%02uV %u.%02uV %3dmV",
             vMainFixed.first,
             vMainFixed.second,
             vBatFixed.first,
             vBatFixed.second,
             vBal);
}

static THD_WORKING_AREA(DISP_WA_SIZE, 256);
THD_FUNCTION(displayThread, )
{
    chThdSetPriority(HIGHPRIO - 1);
    Twi::Init();
    chThdSleepMilliseconds(100);
    Disp::Init();
    Disp::Fill();
    Disp::SetContrast(0);
    chprintf(ds.set2xFontSize(true).getBase(), "  12V UPS");
    chThdSleepSeconds(3);
    Disp::Fill();
    while(true) {
        displayStatus();
        chThdSleepSeconds(1);
    }
}

void run()
{
    auto* thd = chThdCreateStatic(DISP_WA_SIZE, sizeof(DISP_WA_SIZE), NORMALPRIO, displayThread, nullptr);
    chRegSetThreadNameX(thd, "display");
}

} // display
