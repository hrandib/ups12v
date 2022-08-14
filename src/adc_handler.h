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
#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

// clang-format off

#include "hal.h"
#include "chprintf.h"
#include "ch_extended.h"
#include "type_traits_ex.h"

// clang-format on

#include <array>
#include <numeric>

namespace Analog {

template<typename T, size_t size>
class MovingAverageBuf
{
private:
    std::array<T, size> buf_{};
    size_t index{};
public:
    void operator=(T val)
    {
        buf_[index] = val;
        if constexpr(Utils::IsPowerOf2(size)) {
            index = (index + 1) & (size - 1);
        }
        else {
            index = (index + 1) % size;
        }
    }
    operator T()
    {
        return std::accumulate(begin(buf_), end(buf_), (adcsample_t)0) / size;
    }
};

class AdcHandler : Rtos::BaseStaticThread<128>
{
    void main() override;
};

} // Analog

#endif // ADC_HANDLER_H
