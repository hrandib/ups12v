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
#include "circularfifo.h"

// clang-format on

#include <array>
#include <numeric>

namespace Analog {
using namespace Mcudrv;

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
public:
    static constexpr size_t INPUT_CH_NUMBER = 3;
    static constexpr size_t numChannels = INPUT_CH_NUMBER;
    static constexpr size_t dmaBufDepth = 2;
private:
    using sample_buf_t = std::array<adcsample_t, numChannels>;
    using dma_buf_t = std::array<sample_buf_t, dmaBufDepth>;
    using fifo_t = memory_relaxed_acquire_release::CircularFifo<sample_buf_t, 128>;
    using counters_buf_t = std::array<uint32_t, numChannels>;
    using InputPins = InputPinsSequence;

    dma_buf_t dmaBuf_;
    fifo_t fifo_;
    std::array<MovingAverageBuf<adcsample_t, 8>, numChannels> maBuf_;
    ADCDriver& AdcDriver_;
    Rtos::BinarySemaphore semSamples_, semCounters_, semBinaryVal_;
    sample_buf_t samples_;
    counters_buf_t counters_;
    uint16_t binaryVal_;
    static const ADCConversionGroup adcGroupCfg_;
public:
    Input() :
      fifo_{}, maBuf_{}, AdcDriver_{ADCD1}, semSamples_{false}, semCounters_{false},
      semBinaryVal_{false}, samples_{}, counters_{}, binaryVal_{}
    {
        AdcDriver_.customData = this;
    }
    void Init();
    static void AdcCb(ADCDriver* adcp, adcsample_t* buffer, size_t);
    sample_buf_t GetSamples();
    uint16_t GetBinaryVal();
    counters_buf_t GetCounters();
    void main() override;
};

inline Input::sample_buf_t Input::GetSamples()
{
    Rtos::SemLockGuard{semSamples_};
    return samples_;
}

inline uint16_t Input::GetBinaryVal()
{
    Rtos::SemLockGuard{semBinaryVal_};
    return binaryVal_;
}

inline Input::counters_buf_t Input::GetCounters()
{
    Rtos::SemLockGuard{semCounters_};
    return counters_;
}

extern Input input;

} // Analog

#endif // ADC_HANDLER_H
