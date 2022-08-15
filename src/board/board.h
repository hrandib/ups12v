#ifndef BOARD_H
#define BOARD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * Board identifier.
 * https://oshpark.com/shared_projects/HUtPmtCZ
 * Crystal resonator soldered directly on pins
 */
#define BOARD_ST_STM32_F070_MINI
#define BOARD_NAME "stm32-boardtoaster"

#if !defined(STM32_HSECLK)
#define STM32_HSECLK 12000000U
#endif

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD 330U

/*
 * IO pins assignments.
 */
#define GPIOA_ADC0 0U
#define GPIOA_ADC1 1U
#define GPIOA_ADC2 2U
#define GPIOA_ADC3 3U
#define GPIOA_CHRG_EN 4U
#define GPIOA_SCL 5U
#define GPIOA_SDA 6U
#define GPIOA_TRICKLE_EN 7U
#define GPIOA_USB_DM 11U
#define GPIOA_USB_DP 12U
#define GPIOA_SWDIO 13U
#define GPIOA_SWCLK 14U

#define GPIOB_LED 1U

#define GPIOF_OSC_IN 0U
#define GPIOF_OSC_OUT 1U

/*
 * IO lines assignments.
 */
#define LINE_CHRG_EN PAL_LINE(GPIOA, GPIOA_CHRG_EN)
#define LINE_TRICKLE_EN PAL_LINE(GPIOA, GPIOA_TRICKLE_EN)
#define LINE_SWDIO PAL_LINE(GPIOA, GPIOA_SWDIO)
#define LINE_SWCLK PAL_LINE(GPIOA, GPIOA_SWCLK)
#define LINE_OSC_IN PAL_LINE(GPIOF, GPIOF_OSC_IN)
#define LINE_OSC_OUT PAL_LINE(GPIOF, GPIOF_OSC_OUT)
#define LINE_LED PAL_LINE(GPIOB, GPIOB_LED)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n) (0U << ((n)*2U))
#define PIN_MODE_OUTPUT(n) (1U << ((n)*2U))
#define PIN_MODE_ALTERNATE(n) (2U << ((n)*2U))
#define PIN_MODE_ANALOG(n) (3U << ((n)*2U))
#define PIN_ODR_LOW(n) (0U << (n))
#define PIN_ODR_HIGH(n) (1U << (n))
#define PIN_OTYPE_PUSHPULL(n) (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n) (1U << (n))
#define PIN_OSPEED_VERYLOW(n) (0U << ((n)*2U))
#define PIN_OSPEED_LOW(n) (1U << ((n)*2U))
#define PIN_OSPEED_MEDIUM(n) (2U << ((n)*2U))
#define PIN_OSPEED_HIGH(n) (3U << ((n)*2U))
#define PIN_PUPDR_FLOATING(n) (0U << ((n)*2U))
#define PIN_PUPDR_PULLUP(n) (1U << ((n)*2U))
#define PIN_PUPDR_PULLDOWN(n) (2U << ((n)*2U))
#define PIN_AFIO_AF(n, v) ((v) << (((n) % 8U) * 4U))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C"
{
#endif
    void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
