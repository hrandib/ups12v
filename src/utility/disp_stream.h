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

#ifndef DISP_STREAM_H
#define DISP_STREAM_H

#include "cppstreams.h"
#include "fonts.h"

namespace streams {

using namespace Mcucpp;
using Resources::Font;

template<typename Disp>
class DispStream : public BaseSequentialStream
{
private:
    bool fontX2_{};
    const Font* font_{&Resources::font5x8};
public:
    DispStream() = default;
    // BaseSequentialStream interface
    size_t write(const uint8_t* bp, size_t n) override
    {
        (fontX2_ ? Disp::Putbuf2X : Disp::Putbuf)(bp, n, *font_);
        return n;
    }
    size_t read(uint8_t*, size_t) override
    {
        return 0;
    }
    msg_t put(uint8_t b) override
    {
        (fontX2_ ? Disp::Putch2X : Disp::Putch)(b, *font_);
        return MSG_OK;
    }
    msg_t get() override
    {
        return MSG_OK;
    }

    DispStream& set2xFontSize(bool val)
    {
        fontX2_ = val;
        return *this;
    }

    DispStream& setFont(const Font& font)
    {
        font_ = &font;
        return *this;
    }
};

} // streams

#endif // DISP_STREAM_H
