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

#ifndef USB_HELPERS_H
#define USB_HELPERS_H

#include <array>
#include <concepts>
#include <cstdint>

namespace Private {
static constexpr uint8_t USB_DESC_STRING_TYPE = 3U;
template<std::integral T>
consteval auto integer2usbDescriptor(T val)
{
    std::array<uint8_t, sizeof val + 2> arr{};
    arr[0] = arr.size();
    arr[1] = USB_DESC_STRING_TYPE;
    for(unsigned i = 2; i < arr.size(); ++i) {
        arr[i] = val & 0xFF;
        val >>= 8;
    }
    return arr;
}
} // Private

template<std::size_t N>
struct StringDescriptor
{
    // N includes null terminator, the array size will be equal to (real_string_length * 2) + 2
    std::array<uint8_t, (N * 2)> arr{};

    consteval StringDescriptor(const char (&in)[N])
    {
        arr[0] = arr.size();
        arr[1] = Private::USB_DESC_STRING_TYPE;
        auto begin = &arr[2];
        for(unsigned i{}; i < (N - 1); ++i) {
            begin[i * 2] = in[i];
        }
    }
};

template<StringDescriptor desc>
consteval auto operator""_sdesc()
{
    return desc.arr;
}

consteval auto operator""_sdesc16(uint64_t num)
{
    return Private::integer2usbDescriptor(static_cast<uint16_t>(num));
}

#endif // USB_HELPERS_H
