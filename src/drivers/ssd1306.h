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

#ifndef SSD1306_H
#define SSD1306_H

#include "i2c_fallback.h"
#include "fonts.h"
#include "string_utils.h"

namespace Mcudrv {
  using Resources::Color;
  using Resources::Bitmap;
  using Resources::Font;

  struct ssd1306_128x64
  {
    enum {
      Max_X = 127,
      Max_Y = 63,
    };
  protected:
    enum {
      CmdComPinMapValue = 0x12,
      CmdMuxRatioValue = 0x3F
    };
  };
  struct ssd1306_128x32
  {
    enum {
      Max_X = 127,
      Max_Y = 31
    };
  protected:
    enum {
      CmdComPinMapValue = 0x02,
      CmdMuxRatioValue = 0x1F
    };
  };

//SSD1306 driver
  template<typename Twi, typename Type = ssd1306_128x64>
  class ssd1306 : Twi, public Type
  {
  private:
    enum {
      BaseAddr = 0x3C,
      MaxYpages = Type::Max_Y >> 3
    };
    enum ControlByte {
      CtrlCmdSingle = 0x80,
      CtrlCmdStream = 0x00,
      CtrlDataStream = 0x40
    };
    enum InstructionSet {
      CmdSetContrast = 0x81,  //with followed by value 0xCF
      CmdDisplayRam = 0xA4,
      CmdDisplayAllOn = 0xA5,
      CmdDisplayNormal = 0xA6,
      CmdDisplayInverted = 0xA7,
      CmdDisplayOff = 0xAE,
      CmdDisplayOn = 0xAF,

      CmdMemAddrMode = 0x20, //with followed by: 0x00 - Horizontal, 0x01 - Vertical, 0x02 - Page(Default), 0x03 - Invalid
      ModeHorizontal = 0x00,
      ModeVertical = 0x01,
      ModePage = 0x02,

      CmdSetColRange = 0x21, //with followed by column range (0x00 0x7F)
      CmdSetPageRange = 0x22, //with followed by page range (0x00 0x07)

      CmdDisplayStartLine = 0x40, // | 0..63 - Only in Page mode, default: 0x40
      CmdSetPageStart = 0xB0, // | 0..7 - Only in Page mode, default: 0xB0
      CmdSetColStartLower = 0x00, // | 0..15 - Only in Page mode, Lower nibble of column
      CmdSetColStartHigher = 0x10, // | 0..15  - Only in Page mode, Higher nibble of column
      CmdSegmentRemap = 0xA1,
      CmdMuxRatio = 0xA8, //with followed by ratio (default: 0x3F = 64 MUX)
      CmdComScanMode = 0xC8,
      CmdDisplayOffset = 0xD3, //with followed by 0x00
      CmdComPinMap = 0xDA,  //with followed by 0x12
      CmdNop = 0xE3,

      CmdClkDiv = 0xD5,   //with followed by value
      CmdPrecharge = 0xD9,  //with followed by 0xF1
      CmdVComHDeselect = 0xDB,//with followed by 0x30

      CmdChargePump = 0x8D  //with followed by 0x14(internal gen) or 0x10(external gen)
    };
    static const uint8_t initSequence[24];
    static uint8_t x_, y_, prevFontHeight_;
  public:
    static void Init()
    {
      Twi::Write(BaseAddr, initSequence, sizeof(initSequence));
    }
    static void SetContrast(uint8_t con)
    {
      uint8_t seq[3] = { CtrlCmdStream, CmdSetContrast, uint8_t(con & 0x7F) };
      Twi::Write(BaseAddr, seq, sizeof(seq));
    }
    static void SetX(uint8_t x)
    {
      const uint8_t seq[] = { CtrlCmdStream,
                              uint8_t(CmdSetColStartHigher | (x >> 4)),
                              uint8_t(CmdSetColStartLower | (x & 0x0F))};
      Twi::Write(BaseAddr, seq, sizeof(seq));
      x_ = x;
    }
    static void SetY(uint8_t y)
    {
      const uint8_t seq[] = { CtrlCmdSingle, uint8_t(CmdSetPageStart | (y & MaxYpages))};
      Twi::Write(BaseAddr, seq, sizeof(seq));
      y_ = y;
    }
    static void SetXY(uint8_t x, uint8_t y)
    {
      SetX(x);
      SetY(y);
    }
    static void Fill(const Color color = Resources::Clear)
    {
      for(uint8_t y = 0; y <= MaxYpages; ++y) {
        SetXY(0, y);
        Twi::WriteNoStop(BaseAddr, CtrlDataStream);
        for(uint8_t x = 0; x < 128; ++x) {
          Twi::WriteByte(color);
        }
        Twi::Stop();
      }
      SetXY(0, 0);
    }
    static void Fill(uint8_t x, uint8_t xRange, uint8_t y, uint8_t yRange, Color color = Resources::Clear)
    {
      for(uint8_t ypos = 0; ypos < yRange; ++ypos) {
        SetXY(x, y + ypos);
        Twi::WriteNoStop(BaseAddr, CtrlDataStream);
        for(uint8_t xpos = 0; xpos < xRange; ++xpos) {
          Twi::WriteByte(color);
        }
        Twi::Stop();
      }
      SetXY(x + xRange, y);
    }
    static void Draw(const Bitmap& bmap, uint8_t x = x_, uint8_t y = y_)
    {
      for(uint8_t ypos = 0; ypos < (bmap.Height() >> 3); ++ypos) {
        SetXY(x, y + ypos);
        Twi::WriteNoStop(BaseAddr, CtrlDataStream);
        for(uint8_t x = 0; x < bmap.Width(); ++x) {
          Twi::WriteByte(bmap[x + bmap.Width() * ypos]);
        }
        Twi::Stop();
      }
      SetXY(x + bmap.Width(), y);
    }
    static void Draw2X(const Bitmap& bmap, uint8_t x = x_, uint8_t y = y_)
    {
      //buf size should as bigger as width of the biggest font
      uint8_t buf[10];
      uint8_t bufIndex{};
      for(uint8_t ypos = 0; ypos < (bmap.Height() >> 3); ++ypos) {
        SetXY(x, y + (ypos * 2));
        Twi::WriteNoStop(BaseAddr, CtrlDataStream);
        for(uint8_t x = 0; x < bmap.Width(); ++x) {
          uint8_t data = bmap[x + bmap.Width() * ypos];
          uint8_t temp_data{};
          for(uint8_t i{}; i < 4; ++i) {
            if(data & (1 << i)) {
              temp_data |= (3 << (i * 2));
            }
          }
          buf[bufIndex] = 0;
          for(uint8_t i = 4; i < 8; ++i) {
            if(data & (1 << i)) {
              buf[bufIndex] |= (3 << ((i - 4) * 2));
            }
          }
          ++bufIndex;
          Twi::WriteByte(temp_data);
          Twi::WriteByte(temp_data);
        }
        bufIndex = 0;
        Twi::Stop();
        SetXY(x, y + (ypos * 2) + 1);
        Twi::WriteNoStop(BaseAddr, CtrlDataStream);
        for(uint8_t x = 0; x < bmap.Width(); ++x) {
          Twi::WriteByte(buf[x]);
          Twi::WriteByte(buf[x]);
        }
        Twi::Stop();
      }
      SetXY(x + (bmap.Width() * 2), y);
    }

    static bool ProcessSpecialChars(uint8_t ch, uint8_t charHeightInBytes, uint8_t charWidth, uint8_t charSpacing)
    {
      switch(ch) {
      case '\r':
        SetX(0);
        break;
      case '\n':
        SetY(y_ + charHeightInBytes);
        break;
      case '\b':
        SetX(x_ - (charWidth + charSpacing));
        break;
      case ' ':
        Fill(x_, charWidth + charSpacing, y_, charHeightInBytes);
        break;
       default:
        //not processed
        return false;
      }
      //processed
      return true;
    }
    static void AdjustHeightOffset(uint8_t charHeightInBytes)
    {
      int8_t diff = (int8_t)(prevFontHeight_ - charHeightInBytes);
      if(diff) {
        uint8_t ypos = y_;
        if(diff < 0) { //current font bigger than previous
          diff = -diff;
          if(ypos >= diff) {
            ypos -= diff;  //there is a place on top
          }
        }
        else if(diff > 0) { //current font smaller than previous
          ypos += diff;
        }
        SetY(ypos);
        prevFontHeight_ = charHeightInBytes;
      }
    }
    static void Putch(uint8_t ch, const Font& font = Resources::font5x8)
    {
      const uint8_t charHeightInBytes = font.Height() >> 3;
      const uint8_t charWidth = font.Width();
      const uint8_t charSpacing = font.Width() >> 2;
      ProcessSpecialChars(ch, charHeightInBytes, charWidth, charSpacing);
      AdjustHeightOffset(charHeightInBytes);
      //end of line
      if((Type::Max_X - x_) < charWidth) {
        SetXY(0, y_ + charHeightInBytes);
      }
      Draw(Bitmap(font[ch], font.Width(), font.Height()));
      //Space between chars
      Fill(x_, charSpacing, y_, charHeightInBytes);
    }
    static void Putch2X(uint8_t ch, const Font& font = Resources::font5x8)
    {
      const uint8_t charHeightInBytes = font.Height() >> 2;
      const uint8_t charWidth = font.Width() * 2;
      const uint8_t charSpacing = font.Width() >> 1;
      if(ProcessSpecialChars(ch, charHeightInBytes, charWidth, charSpacing)) {
        return;
      }
      AdjustHeightOffset(charHeightInBytes);
      //end of line
      if((Type::Max_X - x_) < charWidth) {
        SetXY(0, y_ + charHeightInBytes);
      }
      Draw2X(Bitmap(font[ch], font.Width(), font.Height()));
      //Space between chars
      Fill(x_, charSpacing, y_, charHeightInBytes);
    }

    static void Puts(const char* str, const Font& font = Resources::font5x8)
    {
      while(*str) {
        Putch(*str++, font);
      }
    }

    static void Puts2X(const char* str, const Font& font = Resources::font5x8)
    {
      while(*str) {
        Putch2X(*str++, font);
      }
    }
  };
  template<typename Twi, typename Type>
  uint8_t ssd1306<Twi, Type>::x_;
  template<typename Twi, typename Type>
  uint8_t  ssd1306<Twi, Type>::y_;
  template<typename Twi, typename Type>
  uint8_t  ssd1306<Twi, Type>::prevFontHeight_ = 1;
  template<typename Twi, typename Type>
  const uint8_t ssd1306<Twi, Type>::initSequence[] = { CtrlCmdStream,
                                                 CmdSetColRange, 0x00, 0x7F,
                                              //  CmdSetPageRange, 0x00, 0x07,
                                                 CmdDisplayOff,  //default
                                              //  CmdClkDiv, 0x80,    //default = 0x80
                                              //  CmdDisplayOffset, 0x00, //default = 0
                                                 CmdMuxRatio, Type::CmdMuxRatioValue,
                                                 CmdChargePump, 0x14,
                                                 CmdMemAddrMode, ModePage, //default = 0x02 (Page)
                                                 CmdSegmentRemap,
                                                 CmdComScanMode,
                                                 CmdComPinMap, Type::CmdComPinMapValue,
                                                 CmdSetContrast, 0xCF,   //default = 0x7F
                                                 CmdPrecharge, 0xF1,
                                                 CmdVComHDeselect, 0x40, //default = 0x20
                                                 CmdDisplayRam,
                                                 CmdDisplayOn
                                               };

}//Mcudrv

#endif // SSD1306_H
