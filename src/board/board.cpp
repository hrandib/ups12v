#include "gpio.h"
#include "hal.h"
#include <concepts>

template<std::unsigned_integral... Positions>
consteval uint16_t getMask(Positions... pos)
{
    return ((1U << pos) | ...);
}

static inline void stm32_gpio_init()
{
    using namespace Mcucpp;
    Gpio::EnablePorts<Porta, Portb>();
    // GPIOA
    Porta::WriteConfig<getMask(GPIOA_ADC0, GPIOA_ADC1, GPIOA_ADC2, GPIOA_ADC3), Gpio::Input, Gpio::Analog>();
    Porta::SetConfig<getMask(GPIOA_SCL, GPIOA_SDA), Gpio::OutputFast, Gpio::OpenDrainPullUp>();
    // TIM14 CH1 PWM
    //  Porta::SetConfig<getMask(GPIOA_CHRG_PWM), Gpio::OutputFast, Gpio::AltPushPull>();
    //  Porta::SetAltFunction<getMask(GPIOA_CHRG_PWM), Gpio::AF::_4>();
    Porta::SetConfig<getMask(GPIOA_CHRG_EN, GPIOA_TRICKLE_EN), Gpio::OutputSlow, Gpio::PushPull>();
    // SWD function restore
    Porta::SetConfig<getMask(GPIOA_SWDIO), Gpio::OutputFast, Gpio::OutputMode(Gpio::AltPushPull | 0x01)>();
    Porta::SetConfig<getMask(GPIOA_SWCLK), Gpio::OutputFast, Gpio::OutputMode(Gpio::AltPushPull | 0x02)>();

    // GPIOB
    Portb::WriteConfig<getMask(GPIOB_LED), Gpio::OutputSlow, Gpio::PushPull>();
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Early initialization code.
 * @details GPIO ports and system clocks are initialized before everything
 *          else.
 */
extern "C" void __early_init()
{
    stm32_gpio_init();
    stm32_clock_init();
}

#if HAL_USE_SDC || defined(__DOXYGEN__)
/**
 * @brief   SDC card detection.
 */
bool sdc_lld_is_card_inserted(SDCDriver* sdcp)
{

    (void)sdcp;
    /* CHTODO: Fill the implementation.*/
    return true;
}

/**
 * @brief   SDC card write protection detection.
 */
bool sdc_lld_is_write_protected(SDCDriver* sdcp)
{

    (void)sdcp;
    /* CHTODO: Fill the implementation.*/
    return false;
}
#endif /* HAL_USE_SDC */

#if HAL_USE_MMC_SPI || defined(__DOXYGEN__)
/**
 * @brief   MMC_SPI card detection.
 */
bool mmc_lld_is_card_inserted(MMCDriver* mmcp)
{

    (void)mmcp;
    /* CHTODO: Fill the implementation.*/
    return true;
}

/**
 * @brief   MMC_SPI card write protection detection.
 */
bool mmc_lld_is_write_protected(MMCDriver* mmcp)
{

    (void)mmcp;
    /* CHTODO: Fill the implementation.*/
    return false;
}
#endif

/**
 * @brief   Board-specific initialization code.
 * @note    You can add your board-specific code here.
 */
void boardInit()
{ }
