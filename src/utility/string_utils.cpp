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

#include <string.h>
#include "string_utils.h"

namespace io {
  uint8_t* InsertDot(uint16_t value, uint8_t position, uint8_t* buf)
  {
    size_t len = strlen((const char*)utoa16(value, buf));
    if(len <= position) {
      uint8_t offset = static_cast<uint8_t>(position + 2 - len);
      memmove(buf + offset, buf, len + 1);
      buf[0] = '0';
      buf[1] = '.';
      for(uint8_t x = 2; x < offset; ++x) {
        buf[x] = '0';
      }
    }
    else { //length > position
      memmove(buf + len - position + 1, buf + len - position, position + 1);
      buf[len - position] = '.';
    }
    return buf;
  }

  // Not so fast as strtoul() (1.2x-2x gap),
  // but thread safe in the embedded environment and can handle binary literals (0b...)
  std::optional<uint32_t> svtou(std::string_view str)
  {
    auto removeLeadingZeros = [&] {
      auto pos = str.find_first_not_of("0");
      if(pos != str.npos) {
        str = str.substr(pos);
      }
      else if(*str.rbegin() == '0') {
        str = str.substr(str.size() - 1);
      }
    };
    auto pos = str.find_first_not_of(" 0");
    if(pos != str.npos) {
      str = str.substr(pos);
      switch(str[0]) {
      case 'x': case 'X': {
          auto isHexDigit = [](char c) {
            return (c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'F') ||
                (c >= 'a' && c <= 'f');
          };
          auto toDigit = [](char c) {
            uint8_t offset =
                (c <= '9') ? '0' :
                             (c <= 'F') ? 'A' - 10 :
                                          'a' - 10;
            return static_cast<uint32_t>(c - offset);
          };
          str.remove_prefix(1);
          removeLeadingZeros();
          if(!str.size() || str.size() > 8) {
            return {};
          }
          uint32_t result{};
          uint32_t rank{1};
          for(auto it = str.rbegin(); it != str.rend(); ++it) {
            if(isHexDigit(*it)) {
              result += toDigit(*it) * rank;
              rank *= 16;
            }
            else {
              return {};
            }
          }
          return result;
        }
      case 'b': case 'B': {
          str.remove_prefix(1);
          removeLeadingZeros();
          uint32_t result{};
          uint32_t rank{1};
          if(!str.size() || str.size() > 32) {
            return {};
          }
          for(auto it = str.rbegin(); it != str.rend(); ++it) {
            uint32_t val = *it == '0' ? 0 :
                          *it == '1' ? 1 : UINT32_MAX;
            if(val == UINT32_MAX) {
              return {};
            }
            result += val * rank;
            rank *= 2;
          }
          return result;
        }
      } //switch end
      removeLeadingZeros();
      if(str.size() > 10) {
        return {};
      }
      uint32_t result{};
      uint32_t rank{1};
      constexpr uint32_t rankMax = 1000'000'000U;
      constexpr uint32_t lim = UINT32_MAX - (rankMax * 4);
      for(auto it = str.rbegin(); it != str.rend(); ++it) {
        if(*it >= '0' && *it <= '9') {
          uint32_t val = *it - '0';
          if(rank == rankMax && (val > 4 || (result > lim && val == 4))) {
            return {};
          }
          result += val * rank;
          rank *= 10;
        }
        else {
          return {};
        }
      }
      return result;
    }
    else if(str.back() == '0') {
      return 0;
    }
    else
      return {};
  }

}//io


