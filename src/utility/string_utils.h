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

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "type_traits_ex.h"
#include <string_view>
#include <optional>

namespace io {
  template<typename T>
  const uint8_t* xtoa(T value, uint8_t* result, uint8_t base = 10)
  {
    typedef typename Utils::make_unsigned<T>::type UT;
    uint8_t* out = result;
    UT quotient;
    if(Utils::is_signed<T>::value) {
      quotient = static_cast<UT>(Utils::is_negative(value) ? -value : value);
    }
    else {
      quotient = static_cast<UT>(value);
    }
    // check that the base if valid
    //  if (base < 2 || base > 36) { *result = '\0'; return NULL; }
    do {
      const UT q = quotient / base;
      const UT rem = quotient - q * base;
      quotient = q;
      *out++ = static_cast<uint8_t>((rem < 10 ? '0' : 'a' - 10) + rem);
    } while(quotient);
    if(Utils::is_signed<T>::value && Utils::is_negative(value)) {
      *out++ = '-';
    }
    *out-- = '\0';
    //reverse string
    uint8_t tmp_char;
    uint8_t* ptr1 = result;
    while(ptr1 < out) {
      tmp_char = *out;
      *out-- = *ptr1;
      *ptr1++ = tmp_char;
    }
    return result;
  }
  inline const char* xtoa(bool val, uint8_t*, uint8_t)
  {
    return val ? "True" : "False";
  }

  template<typename T>
  inline static const uint8_t* utoa(T value, uint8_t* result, uint8_t base = 10)
  {
    static_assert(!Utils::is_signed<T>::value, "utoa called with signed arg");
    return xtoa(value, result, base);
  }
  template<typename T>
  inline static const uint8_t* itoa(T value, uint8_t* result, uint8_t base = 10)
  {
    static_assert(Utils::is_signed<T>::value, "itoa called with unsigned arg");
    return xtoa(value, result, base);
  }

  inline static const uint8_t* utoa8(uint8_t value, uint8_t* result, uint8_t base = 10)
  {
    return utoa(value, result, base);
  }
  inline static const uint8_t* utoa16(uint16_t value, uint8_t* result, uint8_t base = 10)
  {
    return utoa(value, result, base);
  }
  inline static const uint8_t* utoa32(uint32_t value, uint8_t* result, uint8_t base = 10)
  {
    return utoa(value, result, base);
  }

  inline static const uint8_t* itoa8(int8_t value, uint8_t* result, uint8_t base = 10)
  {
    return itoa(value, result, base);
  }
  inline static const uint8_t* itoa16(int16_t value, uint8_t* result, uint8_t base = 10)
  {
    return itoa(value, result, base);
  }
  inline static const uint8_t* itoa32(int32_t value, uint8_t* result, uint8_t base = 10)
  {
    return itoa(value, result, base);
  }

  uint8_t* InsertDot(uint16_t value, uint8_t position, uint8_t* buf);

  std::optional<uint32_t> svtou(std::string_view str);
}//io

#endif // STRING_UTILS_H

