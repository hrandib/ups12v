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

#include "cal_data.h"

constexpr uint16_t CAL_DATA[monitor::AdcChNumber] = {
  1384, // BAT1
  3925, // 12V BUS
  2664  // VBAT
};

constexpr bat_lut_t DISCHARGE_LUT{{{6250, 0},
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

constexpr bat_lut_t CHARGE_LUT{{{7000, 0},
                                {7200, 5},
                                {7400, 10},
                                {7600, 50},
                                {7680, 55},
                                {7740, 60},
                                {7820, 65},
                                {7900, 70},
                                {7970, 75},
                                {8050, 80},
                                {8130, 85},
                                {8220, 90},
                                {8300, 95},
                                {8395, 100}}};
