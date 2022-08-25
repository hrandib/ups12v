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

#ifndef I2C_SENSORS_H
#define I2C_SENSORS_H

#include "i2c_fallback.h"

// Must be defined externally
void delay_ms(uint32_t);

namespace i2c {

enum { BaseAddrLM75 = 0x48, BaseAddr24C = 0x50, BaseAddrBH1750 = 0x23, BaseAddrBMP180 = 0x77 };

template<typename Twi>
class Lm75
{
public:
    enum { BaseAddr = 0x48 };
    static int16_t Read(uint8_t devAddr = 0)
    {
        int16_t result;
        bool success = Twi::Read(BaseAddr | devAddr, (uint8_t*)&result, 2);
        return success ? result / 128 : 0;
    }
};

template<typename Twi>
class Eeprom24c
{
public:
    enum { BaseAddr = 0x50 };
};

template<typename Twi>
class Bh1750
{
public:
    enum { DevAddr = 0x23 };
    enum { BhPowerDown, BhPowerOn, BhReset = 0x07 };
    enum Mode {
        ContHres = 0x10, // 1lx res. typ. 120ms (max 180ms)
        ContHres2,       // 0.5lx res. typ. 120ms
        ContLres         // 4lx res. typ. 16ms (max 24ms)
    };
    static AckState Init(Mode mode = ContHres)
    {
        return AckState(Twi::Write(DevAddr, BhPowerOn) && Twi::Write(DevAddr, mode));
    }
    static void SetMode(Mode mode)
    {
        return Twi::Write(DevAddr, mode);
    }

    static uint16_t Read()
    {
        uint8_t buf[2];
        if(!Twi::Read(DevAddr, buf, 2)) {
            return 0;
        }
        return uint16_t(buf[0] << 8U) | buf[1];
    }
};

template<typename Twi, uint8_t OversamplingFactor = 0>
class Bmp180
{
    static_assert(OversamplingFactor < 4, "Oversampling Factor must be in range 0..3");
private:
    enum {
        BaseAddr = 0x77,
        Oss = OversamplingFactor,
        PMeasureDelay = Oss == 1   ? 8
                        : Oss == 2 ? 14
                        : Oss == 3 ? 26
                                   : 5
    };
    enum RegMap { RegAC1 = 0xAA, RegTestID = 0xD0, RegControl = 0xF4, RegData = 0xF6, RegXlsb = 0xF8 };
    enum CalValues { AC1, AC2, AC3, AC4, AC5, AC6, B1, B2, MB, MC, MD };
    enum ControlValue {
        CmdTemperature = 0x2E,
        CmdPressure = 0x34 | (Oss << 6),
    };

    static uint16_t calArr[11];

    static AckState SendCommand(ControlValue ctrl)
    {
        uint8_t data[2] = {RegControl, ctrl};
        return Twi::Write(BaseAddr, data, 2);
    }
    static uint16_t GetReg(RegMap reg)
    {
        uint16_t result;
        Twi::Write(BaseAddr, (uint8_t)reg, NoStop);
        Twi::Restart();
        Twi::Read(BaseAddr, (uint8_t*)&result, 2);
        return result;
    }
    static void GetCalValues()
    {
        for(uint8_t x = 0; x < 11; ++x) {
            calArr[x] = GetReg(RegMap(RegAC1 + x * 2));
        }
    }
public:
    struct PT
    {
        uint32_t pressure;
        int16_t temperature;
    };
    static void Init()
    {
        GetCalValues();
    }
    template<typename Uart>
    static void PrintCalArray()
    {
        const char* const names[] = {"AC1", "AC2", "AC3", "AC4", "AC5", "AC6", "B1", "B2", "MB", "MC", "MD"};
        for(uint8_t i = 0; i < 11; ++i) {
            Uart::Puts(names[i]);
            Uart::Puts(": ");
            if(i < 3 || i > 5) {
                Uart::Puts(int16_t(calArr[i]));
            }
            else {
                Uart::Puts(calArr[i]);
            }
            Uart::Newline();
        }
    }

    static bool GetValues(PT& pt)
    {
        if(!SendCommand(CmdTemperature)) {
            return false;
        }
        delay_ms(5);
        uint16_t rawvalueT = GetReg(RegData);
        int32_t x1 = ((int32_t)rawvalueT - calArr[AC6]) * calArr[AC5] / (1U << 15);
        int32_t x2 = (int32_t)((int16_t)calArr[MC]) * (1U << 11) / (x1 + calArr[MD]);
        int32_t b5 = x1 + x2;
        pt.temperature = (b5 + 8) / (1U << 4);
        if(!SendCommand(CmdPressure)) {
            return false;
        }
        delay_ms(PMeasureDelay);
        int32_t rawvalueP = GetReg(RegData);
        if(Oss) {
            rawvalueP = (rawvalueP << 8 | (GetReg(RegXlsb)) & 0xFF) >> (8 - Oss);
        }
        int32_t b6 = b5 - 4000;
        x1 = ((int32_t)((int16_t)calArr[B2]) * (b6 * b6 / (1U << 12))) / (1U << 11);
        x2 = (int32_t)((int16_t)calArr[AC2]) * b6 / (1U << 11);
        int32_t x3 = x1 + x2;
        int32_t b3 = ((((int32_t)((int16_t)calArr[AC1]) * 4 + x3) << Oss) + 2) / 4;
        x1 = (int32_t)((int16_t)calArr[AC3]) * b6 / (1U << 13);
        x2 = ((int32_t)((int16_t)calArr[B1]) * ((b6 * b6) / (1U << 12))) / (1UL << 16);
        x3 = ((x1 + x2) + 2) / 4;
        uint32_t b4 = (int32_t)calArr[AC4] * (x3 + 32768U) >> 15U;
        uint32_t b7 = (rawvalueP - b3) * (50000U >> Oss);
        uint32_t p;
        if(b7 < 0x80000000UL) {
            p = (b7 * 2) / b4;
        }
        else {
            p = (b7 / b4) * 2;
        }
        x1 = (p >> 8) * (p >> 8);
        x1 = (x1 * 3038) >> 16;
        x2 = (-7357 * (int32_t)p) >> 16;
        p = p + ((x1 + x2 + 3791) >> 4);
        pt.pressure = p;
        return true;
    }
    static uint32_t GetPressure()
    {
        PT pt;
        GetValues(pt);
        return pt.pressure;
    }

    static int16_t GetTemperature()
    {
        SendCommand(CmdTemperature);
        delay_ms(5);
        uint16_t rawvalue = GetReg(RegData);
        int32_t x1 = ((int32_t)rawvalue - calArr[AC6]) * calArr[AC5] / (1U << 15);
        int32_t x2 = (int32_t)((int16_t)calArr[MC]) * (1U << 11) / (x1 + calArr[MD]);
        return (x1 + x2 + 8) / (1U << 4);
    }
};
template<typename Twi, uint8_t Oss>
uint16_t Bmp180<Twi, Oss>::calArr[11];

template<typename Twi>
class Bmp280
{
public:
    using s32 = int32_t;
    using u32 = uint32_t;
    // ctrl_meas register defs 0xF4
    enum Toversampling { OsTx1 = 1U << 5, OsTx2 = 2U << 5, OsTx4 = 3U << 5, OsTx8 = 4U << 5, OsTx16 = 5U << 5 };
    enum Poversampling {
        OsPx1 = 1U << 2, // Ultra low power
        OsPx2 = 2U << 2, // Low power
        OsPx4 = 3U << 2, // Standard
        OsPx8 = 4U << 2, // High resolution
                         //      OsPx16 = 5U << 2, // Ultra high resolution
    };
    enum Mode { ModeSleep, ModeForced, ModeNormal = 3 };
    // config register defs 0xF5
    enum StandbyTime {
        Stb_500us = 0U << 5,
        Stb_62ms = 1U << 5,
        Stb_125ms = 2U << 5,
        Stb_250ms = 3U << 5,
        Stb_500ms = 4U << 5,
        Stb_1s = 5U << 5,
        Stb_2s = 6U << 5,
        Stb_4s = 7U << 5
    };
    enum IIR {
        IIR_Off = 0U << 2,
        IIR_x1 = 1U << 2,
        IIR_x2 = 2U << 2,
        IIR_x4 = 3U << 2,
        IIR_x8 = 4U << 2,
        IIR_x16 = 5U << 2
    };
private:
    enum { BaseAddr = 0x76, Oss = OsPx8, MeasureDelay = Oss == OsPx1 ? 7 : Oss == OsPx2 ? 9 : Oss == OsPx4 ? 14 : 23 };
    enum RegMap {
        RegID = 0xD0,
        RegControlMeas = 0xF4,
        RegConfig = 0xF5,
        RegPdata = 0xF7,
        RegPxlsb = 0xF9,
        RegTdata = 0xFA,
        RegTxlsb = 0xFC,
        RegCalBegin = 0x88,
        RegDataBegin = RegPdata
    };
    enum CalIndexes { T1, T2, T3, P1, P2, P3, P4, P5, P6, P7, P8, P9 };

    static uint16_t calArr[12];

    static uint16_t ntohs(uint16_t a)
    {
        return ((((a) >> 8) & 0xff) | (((a) << 8) & 0xff00));
    }
    static AckState ForceMeasure()
    {
        uint8_t data[2] = {RegControlMeas, (uint8_t)Oss | OsTx1 | ModeForced};
        return Twi::Write(BaseAddr, data, 2);
    }
    static bool GetData(uint32_t* rawData)
    {
        uint8_t* rawData_u8 = (uint8_t*)rawData;
        Twi::Write(BaseAddr, (uint8_t)RegDataBegin, NoStop);
        Twi::Restart();
        if(Twi::Read(BaseAddr, rawData_u8 + 1, 6)) {
            rawData_u8[0] = 0;
            rawData_u8[7] = rawData_u8[6];
            rawData_u8[6] = rawData_u8[5];
            rawData_u8[5] = rawData_u8[4];
            rawData_u8[4] = 0;
            rawData[0] >>= 4;
            rawData[1] >>= 4;
            return true;
        }
        else {
            return false;
        }
    }
    static void GetCalValues()
    {
        Twi::Write(BaseAddr, (uint8_t)RegCalBegin, NoStop);
        Twi::Restart();
        Twi::Read(BaseAddr, (uint8_t*)&calArr, sizeof(calArr));
        for(uint8_t i = 0; i < sizeof(calArr); ++i) {
            calArr[i] = ntohs(calArr[i]);
        }
    }
public:
    static uint8_t GetID()
    {
        uint8_t id;
        Twi::Write(BaseAddr, (uint8_t)RegID, NoStop);
        Twi::Restart();
        Twi::Read(BaseAddr, &id, 1);
        return id;
    }
    struct PT
    {
        uint32_t pressure;
        int16_t temperature;
    };
    static void Init()
    {
        GetCalValues();
    }
    template<typename Out>
    static void PrintCalArray()
    {
        static const char* const names[] = {"T1", "T2", "T3", "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9"};
        for(uint8_t i = 0; i < sizeof(calArr); ++i) {
            Out::Puts(names[i]);
            Out::Puts(": ");
            if(i == 0 || i == 3) {
                Out::Puts(calArr[i]);
            }
            else {
                Out::Puts((int16_t)calArr[i]);
            }
            Out::Newline();
        }
    }

    static bool GetValues(PT& pt)
    {
        if(ForceMeasure() == NoAck) {
            return false;
        }
        delay_ms(MeasureDelay);
        uint32_t data[2];
        if(GetData((uint32_t*)data)) {
            int32_t t_fine;
            pt.temperature = CompensateTemperature(data[1], t_fine);
            pt.pressure = CompensatePressure(data[0], t_fine);
            return true;
        }
        else {
            return false;
        }
    }

    static s32 CompensateTemperature(s32 uncompT, s32& t_fine)
    {
        /* calculate true temperature*/
        /*calculate x1*/
        s32 v_x1_u32r = ((((uncompT >> 3) - ((s32)calArr[T1] << 1))) * ((s32)s16(calArr[T2]))) >> 11;
        /*calculate x2*/
        s32 v_x2_u32r = (((((uncompT >> 4) - ((s32)calArr[T1])) * ((uncompT >> 4) - ((s32)calArr[T1]))) >> 12) *
                         ((s32)s16(calArr[T3]))) >>
                        14;
        /*calculate t_fine*/
        t_fine = v_x1_u32r + v_x2_u32r;
        /*calculate temperature*/
        return (t_fine * 5 + 128) >> 8;
    }
    static u32 CompensatePressure(s32 uncompP, s32& t_fine)
    {
        /* calculate x1*/
        s32 v_x1_u32r = (t_fine >> 1) - 64000L;
        /* calculate x2*/
        s32 v_x2_u32r = (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 11) * ((s32)s16(calArr[P6]));
        v_x2_u32r = v_x2_u32r + ((v_x1_u32r * ((s32)s16(calArr[P5]))) << 1);
        v_x2_u32r = (v_x2_u32r >> 2) + (((s32)s16(calArr[P4])) << 16);
        /* calculate x1*/
        v_x1_u32r = (((calArr[P3] * (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 13)) >> 3) +
                     ((((s32)s16(calArr[P2])) * v_x1_u32r) >> 1)) >>
                    18;
        v_x1_u32r = ((((32768L + v_x1_u32r)) * ((s32)calArr[P1])) >> 15);
        /* calculate pressure*/
        u32 v_pressure_u32 = (((u32)(1048576L - uncompP) - (v_x2_u32r >> 12))) * 3125;
        /* Avoid exception caused by division by zero */
        if(v_x1_u32r != 0) {
            /* check overflow*/
            if(v_pressure_u32 < 0x80000000UL) {
                v_pressure_u32 = (v_pressure_u32 << 1) / ((u32)v_x1_u32r);
            }
            else {
                v_pressure_u32 = (v_pressure_u32 / (u32)v_x1_u32r) * 2;
            }
        }
        else {
            return 0;
        }
        /* calculate x1*/
        v_x1_u32r = (((s32)s16(calArr[P9])) * ((s32)(((v_pressure_u32 >> 3) * (v_pressure_u32 >> 3)) >> 13))) >> 12;
        /* calculate x2*/
        v_x2_u32r = (((s32)(v_pressure_u32 >> 2)) * ((s32)s16(calArr[P8]))) >> 13;
        /* calculate true pressure*/
        v_pressure_u32 = (u32)((s32)v_pressure_u32 + ((v_x1_u32r + v_x2_u32r + s16(calArr[P7])) >> 4));
        return v_pressure_u32;
    }
};
template<typename Twi>
uint16_t Bmp280<Twi>::calArr[12];

template<typename Twi>
class Hdc1080
{
private:
    enum { BaseAddr = 0x40, MeasureDelay = 20 };
    enum Mode {
        Tres14bit = 0U << 10,
        Tres11bit = 1U << 10,
        Hres14bit = 0U << 8,
        Hres11bit = 1U << 8,
        Hres8bit = 2U << 8
    };
public:
    struct HT
    {
        int16_t temperature;
        uint16_t humidity;
    };

    static bool GetValues(HT& ht)
    {
        if(Ack != Twi::Write(BaseAddr, 0)) {
            return false;
        }
        delay_ms(MeasureDelay);
        if(Ack != Twi::Read(BaseAddr, (uint8_t*)&ht, 4)) {
            return false;
        }
        ht.temperature = int16_t((uint32_t(ht.temperature) * 1650) >> 16) - 400;
        ht.humidity = (uint32_t(ht.humidity) * 1000) >> 16;
        return true;
    }
};

} // i2c

#endif // I2C_SENSORS_H
