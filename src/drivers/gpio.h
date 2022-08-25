#pragma once

#include "stm32f0xx.h"
#include <concepts>
#include <type_traits>

namespace Mcucpp {

template<std::unsigned_integral... Positions>
consteval uint16_t getMask(Positions... pos)
{
    return ((1U << pos) | ...);
}

namespace Gpio {
enum InputConf { Input };
enum InputMode {
    Analog = 0x18,
    Floating = 0x00,
    PullUp = 0x01,
    PullDown = 0x02,
};
enum OutputConf {
    // 2 MHz
    OutputSlow,
    // 10 MHz
    OutputFast,
    // 50 MHz
    OutputFastest = 0x03
};
enum OutputMode {
    PushPull = 0x08,
    OpenDrain = 0x0C,
    OpenDrainPullUp = 0x0D,
    AltPushPull = 0x10,
    AltOpenDrain = 0x14,
    AltOpenDrainPullUp = 0x15,
};
enum class AF { _0, _1, _2, _3, _4, _5, _6, _7 };

namespace Private {

typedef uint16_t DataT;

template<unsigned mask>
class ConfigurationMask4bit
{
    static const unsigned mask1 = (mask & 0xf0) << 12 | (mask & 0x0f);
    static const unsigned mask2 = (mask1 & 0x000C000C) << 6 | (mask1 & 0x00030003);
    static const unsigned mask3 = (mask2 & 0x02020202) << 3 | (mask2 & 0x01010101);
public:
    static const unsigned value = mask3;
};

constexpr unsigned UnpackConfig4bit(unsigned mask, unsigned value, unsigned configuration)
{
    mask = (mask & 0xf0) << 12 | (mask & 0x0f);
    mask = (mask & 0x000C000C) << 6 | (mask & 0x00030003);
    mask = (mask & 0x02020202) << 3 | (mask & 0x01010101);
    return (value & ~(mask * 0x15)) | mask * configuration;
}

template<unsigned mask>
class ConfigurationMask2bit
{
    static const unsigned mask1 = (mask & 0xff00) << 8 | (mask & 0x00ff);
    static const unsigned mask2 = (mask1 & 0x00f000f0) << 4 | (mask1 & 0x000f000f);
    static const unsigned mask3 = (mask2 & 0x0C0C0C0C) << 2 | (mask2 & 0x03030303);
    static const unsigned mask4 = (mask3 & 0x22222222) << 1 | (mask3 & 0x11111111);
public:
    static const unsigned value = mask4;
};

constexpr unsigned UnpackConfig2bit(unsigned mask, unsigned value, unsigned configuration)
{
    mask = (mask & 0xff00) << 8 | (mask & 0x00ff);
    mask = (mask & 0x00f000f0) << 4 | (mask & 0x000f000f);
    mask = (mask & 0x0C0C0C0C) << 2 | (mask & 0x03030303);
    mask = (mask & 0x22222222) << 1 | (mask & 0x11111111);
    return (value & ~(mask * 0x03)) | mask * configuration;
}

template<uint32_t baseaddr, uint32_t ID>
class PortImplementation
{
private:
    constexpr static inline GPIO_TypeDef* Regs()
    {
        return reinterpret_cast<GPIO_TypeDef*>(baseaddr);
    }
public:
    enum { id = ID };
    inline static void Set(DataT value)
    {
        Regs()->BSRR = value;
    }
    inline static void Clear(DataT value)
    {
        Regs()->BRR = value;
    }
    inline static void ClearAndSet(DataT clearMask, DataT value)
    {
        Regs()->BSRR = (value | (uint32_t)clearMask << 16);
    }
    inline static void Toggle(DataT value)
    {
        Regs()->ODR ^= value;
    }
    inline static void Write(DataT value)
    {
        Regs()->ODR = value;
    }
    inline static DataT Read()
    {
        return Regs()->IDR;
    }
    inline static DataT ReadODR()
    {
        return Regs()->ODR;
    }
    inline static GPIO_TypeDef* GetRegs()
    {
        return Regs();
    }

    // constant interface

    template<DataT value>
    inline static void Set()
    {
        Regs()->BSRR = value;
    }
    template<DataT value>
    inline static void Clear()
    {
        Regs()->BRR = value;
    }
    template<DataT clearMask, DataT value>
    inline static void ClearAndSet()
    {
        Regs()->BSRR = value | (uint32_t)clearMask << 16;
    }
    template<DataT value>
    inline static void Toggle()
    {
        Regs()->ODR ^= value;
    }
    template<DataT value>
    inline static void Write()
    {
        Regs()->ODR = value;
    }

    // end of constant interface

    // all pins except mask will be inputs with pull down
    template<DataT mask, OutputConf speed, OutputMode mode>
    inline static void WriteConfig()
    {
        enum { mask2bit = ConfigurationMask2bit<mask>::value };
        Regs()->MODER = (mode >> 3) * mask2bit;
        Regs()->OSPEEDR = (uint32_t)speed * mask2bit;
        Regs()->OTYPER = ((mode >> 2) & 0x01) * mask;
        Regs()->PUPDR = (mode & 0x03) * mask2bit | (~mask2bit) * PullDown;
    }
    template<DataT mask, OutputConf speed, OutputMode mode>
    inline static void SetConfig()
    {
        enum { mask2bit = ConfigurationMask2bit<mask>::value };
        Regs()->MODER = (Regs()->MODER & ~(mask2bit * 0x03)) | (mode >> 3) * mask2bit;
        Regs()->OTYPER = (Regs()->OTYPER & ~mask) | ((mode >> 2) & 0x01) * mask;
        Regs()->OSPEEDR = (Regs()->OSPEEDR & ~(mask2bit * 0x03)) | (uint32_t)speed * mask2bit;
        Regs()->PUPDR = (Regs()->PUPDR & ~(mask2bit * 0x03)) | (mode & 0x03) * mask2bit;
    }
    // all pins except mask will be inputs with pull down
    template<DataT mask, InputConf, InputMode mode>
    inline static void WriteConfig()
    {
        enum { mask2bit = ConfigurationMask2bit<mask>::value };
        Regs()->MODER = (mode >> 3) * mask2bit;
        Regs()->PUPDR = (mode & 0x03) * mask2bit | (~mask2bit) * PullDown;
    }
    template<DataT mask, InputConf, InputMode mode>
    inline static void SetConfig()
    {
        enum { mask2bit = ConfigurationMask2bit<mask>::value };
        Regs()->MODER = (Regs()->MODER & ~(mask2bit * 0x03)) | (mode >> 3) * mask2bit;
        Regs()->PUPDR = (Regs()->PUPDR & ~(mask2bit * 0x03)) | (mode & 0x03) * mask2bit;
    }
    template<typename Conf, typename Mode>
    inline static void SetConfig(DataT mask, Conf conf, Mode mode)
    {
        static_assert((std::is_same<InputConf, Conf>::value && std::is_same<InputMode, Mode>::value) ||
                        (std::is_same<OutputConf, Conf>::value && std::is_same<OutputMode, Mode>::value),
                      "SetConfig args error");
        if(std::is_same<OutputConf, Conf>::value) {
            Regs()->OSPEEDR = UnpackConfig2bit(mask, Regs()->OSPEEDR, conf);
            Regs()->OTYPER = (Regs()->OTYPER & ~mask) | ((mode >> 2) & 0x01) * mask;
        }
        Regs()->MODER = UnpackConfig2bit(mask, Regs()->MODER, mode >> 3);
        Regs()->PUPDR = UnpackConfig2bit(mask, Regs()->PUPDR, mode & 0x03);
    }
    inline static void SetSpeed(DataT mask, OutputConf speed)
    {
        Regs()->OSPEEDR = UnpackConfig2bit(mask, Regs()->OSPEEDR, speed);
    }
    inline static void SetPUPD(DataT mask, InputMode pull)
    {
        Regs()->PUPDR = UnpackConfig2bit(mask, Regs()->PUPDR, pull & 0x03);
    }
    inline static void SetDriverType(DataT mask, OutputMode mode)
    {
        Regs()->OTYPER = (Regs()->OTYPER & ~mask) | ((mode >> 2) & 0x01) * mask;
    }
    template<DataT mask, AF af_>
    inline static void SetAltFunction()
    {
        constexpr uint32_t af = static_cast<uint32_t>(af_);
        static_assert((baseaddr == GPIOA_BASE && af < 8) || (baseaddr == GPIOB_BASE && af < 4),
                      "Wrong alt function number (or port)");
        enum {
            mask4bitLow = ConfigurationMask4bit<mask & 0xFF>::value,
            mask4bitHigh = ConfigurationMask4bit<(mask >> 8) & 0xFF>::value
        };
        if(mask & 0xFF)
            Regs()->AFR[0] = (Regs()->AFR[0] & ~mask4bitLow) | af * mask4bitLow;
        if((mask >> 8) & 0xFF)
            Regs()->AFR[1] = (Regs()->AFR[1] & ~mask4bitHigh) | af * mask4bitHigh;
    }
    inline static void SetAltFunction(DataT mask, uint8_t number)
    {
        if(mask & 0xFF)
            Regs()->AFR[0] = UnpackConfig4bit(mask & 0xFF, Regs()->AFR[0], number);
        if((mask >> 8) & 0xFF)
            Regs()->AFR[1] = UnpackConfig4bit((mask >> 8) & 0xff, Regs()->AFR[1], number);
    }
    inline static void Enable()
    {
        STATIC_ASSERT(id < 6);
        RCC->AHBENR |= RCC_AHBENR_GPIOAEN << id;
    }
    inline static void Disable()
    {
        STATIC_ASSERT(id < 6);
        RCC->AHBENR &= ~(RCC_AHBENR_GPIOAEN << id);
    }
};

struct NullPort
{
    enum { id = 0xFF };
    inline static void Set(DataT)
    { }
    inline static void Clear(DataT)
    { }
    inline static void ClearAndSet(DataT, DataT)
    { }
    inline static void Toggle(DataT)
    { }
    inline static void Write(DataT)
    { }
    // constant interface
    template<DataT>
    inline static void Set()
    { }
    template<DataT>
    inline static void Clear()
    { }
    template<DataT, DataT>
    inline static void ClearAndSet()
    { }
    template<DataT>
    inline static void Toggle()
    { }
    template<DataT>
    inline static void Write()
    { }
    // end of constant interface

    template<DataT, OutputConf, OutputMode> // all pins except mask will be inputs
                                            // with pull down
    static void WriteConfig()
    { }
    template<DataT, InputConf, InputMode> // all pins except mask will be inputs
                                          // with pull down
    static void WriteConfig()
    { }
    template<DataT, OutputConf, OutputMode>
    inline static void SetConfig()
    { }
    template<DataT, InputConf, InputMode>
    inline static void SetConfig()
    { }
    inline static void SetSpeed(DataT, OutputConf)
    { }
    inline static void SetPullUp(DataT, InputMode)
    { }
    inline static void SetDriverType(DataT, OutputMode)
    { }
    inline static void Enable()
    { }
    inline static void Disable()
    { }

    template<DataT, AF>
    inline static void AltFuncNumber()
    { }
    inline static void AltFuncNumber(DataT, uint8_t)
    { }
};

enum class Trigger { No, RisingEdge, FallingEdge, BothEdges };

template<typename Pin>
class ExtiImplementation
{
private:
    static constexpr IRQn_Type NVIC_IRQ = Pin::position < 2   ? EXTI0_1_IRQn
                                          : Pin::position < 4 ? EXTI2_3_IRQn
                                                              : EXTI4_15_IRQn;
    static void SetTriggerEdge(Trigger tr)
    {
        const uint32_t tr_ = static_cast<uint32_t>(tr);
        EXTI->RTSR |= (tr_ & 0x01 ? Pin::mask : 0);
        EXTI->FTSR |= (tr_ & 0x02 ? Pin::mask : 0);
    }
public:
    static void EnableIRQ(Trigger tr = Trigger::RisingEdge)
    {
        EXTI->IMR |= Pin::mask;
        SetTriggerEdge(tr);
        NVIC_EnableIRQ(NVIC_IRQ);
        SYSCFG->EXTICR[Pin::position / 4] |= Pin::port_id << ((Pin::position % 4) * 4);
    }
    static void DisableIRQ()
    {
        EXTI->IMR &= ~Pin::mask;
        EXTI->RTSR &= ~Pin::mask;
        EXTI->FTSR &= ~Pin::mask;
        NVIC_DisableIRQ(NVIC_IRQ);
        SYSCFG->EXTICR[Pin::position / 4] &= ~(Pin::port_id << ((Pin::position % 4) * 4));
    }
    static void EnableEvent(Trigger tr = Trigger::RisingEdge)
    {
        EXTI->EMR |= Pin::mask;
        SetTriggerEdge(tr);
    }
    static void DisableEvent()
    {
        EXTI->EMR &= ~Pin::mask;
        EXTI->RTSR &= ~Pin::mask;
        EXTI->FTSR &= ~Pin::mask;
    }
    static void ClearPending()
    {
        EXTI->PR = Pin::mask;
    }
    static void SetPriority(uint8_t prio)
    {
        NVIC_SetPriority(NVIC_IRQ, prio);
    }
};

template<typename PORT, uint16_t pos>
class TPin
{
private:
    using Self = TPin<PORT, pos>;
public:
    typedef PORT Port;
    enum { position = pos, mask = 1 << pos, port_id = Port::id };
    using Exti = ExtiImplementation<Self>;

    template<OutputConf conf, OutputMode mode>
    inline static void SetConfig()
    {
        Port::template SetConfig<mask, conf, mode>();
    }

    template<InputConf conf, InputMode mode>
    inline static void SetConfig()
    {
        Port::template SetConfig<mask, conf, mode>();
    }
    template<typename Conf, typename Mode>
    inline static void SetConfig(Conf conf, Mode mode)
    {
        Port::SetConfig(mask, conf, mode);
    }

    template<AF altfunc>
    inline static void AltFuncNumber()
    {
        Port::template AltFuncNumber<mask, altfunc>();
    }
    inline static void AltFuncNumber(uint8_t number)
    {
        Port::AltFuncNumber(mask, number);
    }

    inline static void Set()
    {
        Port::template Set<mask>();
    }
    inline static void Clear()
    {
        Port::template Clear<mask>();
    }
    inline static void SetOrClear(bool cond)
    {
        if(cond)
            Set();
        else
            Clear();
    }
    inline static void Toggle()
    {
        Port::template Toggle<mask>();
    }
    inline static bool IsSet()
    {
        return Port::Read() & mask;
    }
    inline static bool IsSetODR()
    {
        return Port::ReadODR() & mask;
    }
};

template<typename... ports>
struct PortsEnableMask;
template<typename First, typename... Rest>
struct PortsEnableMask<First, Rest...>
{
    static_assert(First::id < 6, "This port not present");
    enum { value = RCC_AHBENR_GPIOAEN << First::id | PortsEnableMask<Rest...>::value };
};
template<>
struct PortsEnableMask<>
{
    enum { value = 0 };
};

} // Private

typedef Private::NullPort NullPort;

template<typename First, typename... Rest>
inline void EnablePorts()
{
    using namespace Private;
    RCC->AHBENR |= PortsEnableMask<First, Rest...>::value;
}

template<typename Pin>
struct Inverted : public Pin
{
    static void Set()
    {
        Pin::Clear();
    }
    static void Clear()
    {
        Pin::Set();
    }
};

using Private::Trigger;
template<typename Pin>
using Exti = Private::ExtiImplementation<Pin>;

constexpr Private::DataT ALL_PINS = 0xFFFF;

constexpr uint16_t P0 = 0x01;
constexpr uint16_t P1 = 0x02;
constexpr uint16_t P2 = 0x04;
constexpr uint16_t P3 = 0x08;
constexpr uint16_t P4 = 0x10;
constexpr uint16_t P5 = 0x20;
constexpr uint16_t P6 = 0x40;
constexpr uint16_t P7 = 0x80;
constexpr uint16_t P8 = 0x0100;
constexpr uint16_t P9 = 0x0200;
constexpr uint16_t P10 = 0x0400;
constexpr uint16_t P11 = 0x0800;
constexpr uint16_t P12 = 0x1000;
constexpr uint16_t P13 = 0x2000;
constexpr uint16_t P14 = 0x4000;
constexpr uint16_t P15 = 0x8000;

} // Gpio

#define PORTDEF(x, y, z) typedef Gpio::Private::PortImplementation<GPIO##x##_BASE, z> Port##y

PORTDEF(A, a, 0);
PORTDEF(B, b, 1);
PORTDEF(C, c, 2);
PORTDEF(D, d, 3);
#if !defined(STM32F070x6)
PORTDEF(E, e, 4);
#endif
PORTDEF(F, f, 5);
typedef Gpio::Private::NullPort NullPort; // null port

#define PINSDEF(x)                                     \
    typedef Gpio::Private::TPin<Port##x, 0> P##x##0;   \
    typedef Gpio::Private::TPin<Port##x, 1> P##x##1;   \
    typedef Gpio::Private::TPin<Port##x, 2> P##x##2;   \
    typedef Gpio::Private::TPin<Port##x, 3> P##x##3;   \
    typedef Gpio::Private::TPin<Port##x, 4> P##x##4;   \
    typedef Gpio::Private::TPin<Port##x, 5> P##x##5;   \
    typedef Gpio::Private::TPin<Port##x, 6> P##x##6;   \
    typedef Gpio::Private::TPin<Port##x, 7> P##x##7;   \
    typedef Gpio::Private::TPin<Port##x, 8> P##x##8;   \
    typedef Gpio::Private::TPin<Port##x, 9> P##x##9;   \
    typedef Gpio::Private::TPin<Port##x, 10> P##x##10; \
    typedef Gpio::Private::TPin<Port##x, 11> P##x##11; \
    typedef Gpio::Private::TPin<Port##x, 12> P##x##12; \
    typedef Gpio::Private::TPin<Port##x, 13> P##x##13; \
    typedef Gpio::Private::TPin<Port##x, 14> P##x##14; \
    typedef Gpio::Private::TPin<Port##x, 15> P##x##15;

PINSDEF(a)
PINSDEF(b)
PINSDEF(c)
PINSDEF(d)
#if !defined(STM32F070x6)
PINSDEF(e)
#endif
PINSDEF(f)

typedef Gpio::Private::TPin<NullPort, 0x00> Nullpin;

} // Mcucpp
