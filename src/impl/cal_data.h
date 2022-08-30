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

#ifndef CAL_DATA_H
#define CAL_DATA_H

#include "monitor.h"
#include <cstdint>
#include <tuple>

// Numerator/Denominator pairs
// Evalutated denominator should be additionally multiplied by the buf depth
constexpr uint16_t CAL_DATA[monitor::AdcChNumber] = {
  1384, // BAT1
  3925, // 12V BUS
  2664  // VBAT
};
constexpr uint32_t FULL_SCALE = 4095U;

using bat_lut_t = std::array<std::pair<uint16_t, uint16_t>, 14>;
static constexpr bat_lut_t DISCHARGE_LUT{{{6250, 0},
                                          {6750, 6},
                                          {6970, 12},
                                          {7100, 18},
                                          {7210, 24},
                                          {7280, 30},
                                          {7335, 36},
                                          {7380, 42},
                                          {7420, 48},
                                          {7460, 54},
                                          {7510, 60},
                                          {7560, 66},
                                          {7630, 72},
                                          {7950, 100}}};

static constexpr bat_lut_t CHARGE_LUT{{{3500 * 2, 0},
                                       {3600 * 2, 5},
                                       {3700 * 2, 10},
                                       {3840 * 2, 50},
                                       {3850 * 2, 55},
                                       {3870 * 2, 60},
                                       {3910 * 2, 65},
                                       {3950 * 2, 70},
                                       {3980 * 2, 75},
                                       {4020 * 2, 80},
                                       {4080 * 2, 85},
                                       {4110 * 2, 90},
                                       {4150 * 2, 95},
                                       {4195 * 2, 100}}};
#endif // CAL_DATA_H
