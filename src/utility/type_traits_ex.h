/*
 * Copyright (c) 2017 Dmytro Shestakov
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
#ifndef TYPE_TRAITS_EX_H
#define TYPE_TRAITS_EX_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Utils {

using std::is_signed;
using std::is_unsigned;
using std::make_signed;
using std::make_unsigned;

template<typename T>
static constexpr bool is_signed_v = std::is_signed<T>::value;
template<typename T>
static constexpr bool is_unsigned_v = std::is_unsigned<T>::value;

template<uint32_t val>
struct Int2Type
{
    enum { value = val };
};

template<size_t sizeBits>
struct SelectSize
{
    static constexpr bool LessOrEq8 = sizeBits <= 8;
    static constexpr bool LessOrEq16 = sizeBits <= 16;
    static constexpr bool LessOrEq32 = sizeBits <= 32;

    using type = std::conditional_t<LessOrEq8, uint8_t, std::conditional_t<LessOrEq16, uint16_t, uint32_t>>;
};

template<size_t sizeBits>
using SelectSize_t = typename SelectSize<sizeBits>::type;

template<unsigned size>
struct SelectSizeForLength
{
    static const bool LessOrEq8 = size <= 0xff;
    static const bool LessOrEq16 = size <= 0xffff;

    using type = std::conditional_t<LessOrEq8, uint8_t, std::conditional_t<LessOrEq16, uint16_t, uint32_t>>;
};

template<typename T>
std::enable_if_t<is_unsigned_v<T>, bool> is_negative(T)
{
    return false;
}
template<typename T>
std::enable_if_t<is_signed_v<T>, bool> is_negative(T value)
{
    return value < 0;
}

// Pinlist helpers

// NumberToMask<3>::value == 0b0111;
template<uint32_t Num>
struct NumberToMask
{
    enum { value = 1 << (Num - 1) | NumberToMask<Num - 1>::value };
};
template<>
struct NumberToMask<0>
{
    enum { value = 0 };
};

template<uint32_t Num>
constexpr uint32_t NumberToMask_v = NumberToMask<Num>::value;

// MaskToPosition<0b0100>::value == 0x02;
template<uint32_t mask>
struct MaskToPosition
{
    enum { value = MaskToPosition<(mask >> 1)>::value + 1 };
};
template<>
struct MaskToPosition<0x01>
{
    enum { value = 0 };
};

// Need for Nullpin
template<>
struct MaskToPosition<0x00>
{
    enum { value = 0 };
};

template<uint32_t mask>
constexpr uint32_t MaskToPosition_v = MaskToPosition<mask>::value;

static constexpr uint32_t Unpack2Bit(uint32_t mask)
{
    mask = (mask & 0xff00) << 8 | (mask & 0x00ff);
    mask = (mask & 0x00f000f0) << 4 | (mask & 0x000f000f);
    mask = (mask & 0x0C0C0C0C) << 2 | (mask & 0x03030303);
    mask = (mask & 0x22222222) << 1 | (mask & 0x11111111);
    return mask;
}
static constexpr uint32_t Unpack4Bit(uint32_t mask)
{
    mask = (mask & 0xf0) << 12 | (mask & 0x0f);
    mask = (mask & 0x000C000C) << 6 | (mask & 0x00030003);
    mask = (mask & 0x02020202) << 3 | (mask & 0x01010101);
    return mask;
}
static constexpr uint32_t Unpack3Bit(uint32_t mask)
{
    mask = (mask & 0x0FC0) << 12 | (mask & 0x003F);
    mask = (mask & 0x38E38E38) << 6 | (mask & 0xC71C71C7);
    mask = (mask & 0xA4924924) << 4 | (mask & 0xDB6DB6DB);
    mask = (mask & 0x12492492) << 2 | (mask & 0x49249249);
    return mask;
}

static constexpr bool IsPowerOf2(uint32_t val)
{
    return val && !(val & (val - 1));
}
}

#endif // TYPE_TRAITS_H
