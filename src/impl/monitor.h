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

#ifndef MONITOR_H
#define MONITOR_H

#include <atomic>

namespace monitor {

// AdcBat1 is a single element of 2S battery connected to GND
enum AdcChannels { AdcBat1, AdcMain, AdcVBat, AdcChNumber };

using std::atomic_uint16_t;
using adc_data_t = atomic_uint16_t[AdcChNumber];

// 85% battery charge by default;
extern atomic_uint16_t chargeCutoff;
// 55% battery charge by default
extern atomic_uint16_t idleDischargeCutoff;

enum class State : uint16_t { Idle, Trickle, Discharge, Charge };
extern std::atomic<State> state;
extern const char* stateString[];
static inline const char* toString(decltype(state)& state)
{
    return stateString[(uint16_t)state.load(std::memory_order_relaxed)];
}

extern adc_data_t voltages;

void run();

} // data

#endif // MONITOR_H