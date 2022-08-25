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
#ifndef I2C_FALLBACK_H
#define I2C_FALLBACK_H

#include "gpio.h"

namespace i2c {

using namespace Mcucpp;

enum Mode { Standard, Fast };
enum AddrType { Addr7bit, Addr10bit };
enum StopMode { Stop, NoStop };
enum AckState { NoAck, Ack };

template<uint8_t NOPS>
static inline void nops()
{
    __NOP();
    nops<NOPS - 1>();
}
template<>
inline void nops<0>()
{ }

template<typename Scl, typename Sda, uint8_t nops_delay = 3>
class SoftTwi

{
private:
    static void Delay()
    {
        nops<nops_delay>();
    }

    static bool Release()
    {
        for(uint8_t scl = 0; scl < 10; ++scl) {
            Scl::Clear();
            Delay();
            Scl::Set();
            Delay();
            if(Sda::IsSet()) {
                Stop();
                return true; // Sda released
            }
        }
        return false; // Line is still busy
    }
protected:
    static void Start()
    {
        Sda::Clear();
        Delay();
        Scl::Clear();
        Delay();
    }
    static void Stop()
    {
        Scl::Clear();
        Delay();
        Sda::Clear();
        Delay();
        Scl::Set();
        Delay();
        Sda::Set();
    }
    static AckState WriteByte(uint8_t data)
    {
        AckState ack = Ack;
        for(uint8_t i = 0; i < 8; ++i) {
            if((data & 0x80) == 0) {
                Sda::Clear();
            }
            else {
                Sda::Set();
            }
            Delay();
            Scl::Set();
            Delay();
            Scl::Clear();
            data <<= 1U;
        }
        Sda::Set();
        Delay();
        Scl::Set();
        Delay();
        if(Sda::IsSet()) {
            ack = NoAck;
        }
        else {
            ack = Ack;
        }
        Scl::Clear();
        return ack;
    }
    static uint8_t ReadByte(AckState ackstate = Ack)
    {
        uint8_t data = 0;
        Sda::Set();
        for(uint8_t i = 0; i < 8; ++i) {
            data = uint8_t(data << 1);
            Scl::Set();
            Delay();
            if(Sda::IsSet()) {
                data |= 0x01;
            }
            Scl::Clear();
            Delay();
        }
        if(ackstate == Ack) {
            Sda::Clear();
        }
        else {
            Sda::Set();
        }
        Delay();
        Scl::Set();
        Delay();
        Scl::Clear();
        Delay();
        Sda::Set();
        return data;
    }
public:
    static bool Init()
    {
        using namespace Gpio;
        if constexpr((uint16_t)Scl::port_id == (uint16_t)Sda::port_id) {
            Scl::Port::Set((uint16_t)Scl::mask | (uint16_t)Sda::mask);
            Scl::Port::template SetConfig<(uint16_t)Scl::mask | (uint16_t)Sda::mask, OutputFast, OpenDrainPullUp>();
        }
        else {
            Scl::Set();
            Scl::template SetConfig<OutputFast, OpenDrainPullUp>();
            Sda::Set();
            Sda::template SetConfig<OutputFast, OpenDrainPullUp>();
        }
        if(!Sda::IsSet()) {
            return Release(); // Reset slave devices
        }
        return true; // Bus Ready
    }
    static void Restart()
    {
        Sda::Set();
        Delay();
        Scl::Set();
        Delay();
    }

    static AckState WriteNoStop(uint8_t addr, const uint8_t* buf, uint8_t length)
    {
        Start();
        AckState state = WriteByte(addr << 1);
        if(state != NoAck) {
            while(length--) {
                if(WriteByte(*buf++) == NoAck) {
                    break;
                }
            }
            if(!length) {
                state = Ack;
            }
        }
        return state;
    }
    static AckState Write(uint8_t addr, const uint8_t* buf, uint8_t length)
    {
        auto state = WriteNoStop(addr, buf, length);
        Stop();
        return state;
    }
    static AckState WriteNoStop(const uint8_t* buf, uint8_t length) // length of data (except address)
    {
        return WriteNoStop(*buf, buf + 1, length);
    }
    static AckState Write(const uint8_t* buf, uint8_t length) // length of data (except address)
    {
        return Write(*buf, buf + 1, length);
    }
    static AckState WriteNoStop(uint8_t addr, uint8_t data)
    {
        Start();
        AckState state = NoAck;
        if(WriteByte(addr << 1U) == Ack && WriteByte(data) == Ack) {
            state = Ack;
        }
        return state;
    }
    static AckState Write(uint8_t addr, uint8_t data)
    {
        auto state = WriteNoStop(addr, data);
        Stop();
        return state;
    }
    static bool Read(uint8_t addr, uint8_t* buf, uint8_t length)
    {
        Start();
        bool result = false;
        if(WriteByte((addr << 1U) | 0x01)) {
            while(--length) {
                *buf++ = ReadByte();
            }
            *buf = ReadByte(NoAck);
            result = true;
        }
        Stop();
        return result;
    }
};

} // i2c

#endif // I2C_FALLBACK_H
