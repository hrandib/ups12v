/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "portab.h"

#include "chprintf.h"
#include "shell.h"

#include "usbcfg.h"

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2048)

const static uint8_t buf1[512] = "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "ZXCVBNM<>?ASDFGHJK:QWRTYUIOP9876543210-ZXCVBNM<>?ASDFGHJK:QWRTY\r\n"
                                 "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXY\r\n";

const static uint8_t buf0[512] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\r\n"
                                 "0123456789abcdef0123456789abcdef0123456789abcdef012345\r\n";

///* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
// static void cmd_write(BaseSequentialStream* chp, int argc, char* argv[])
//{

//    (void)argv;
//    if(argc > 0) {
//        chprintf(chp, "Usage: write\r\n");
//        return;
//    }

//    while(chnGetTimeout((BaseChannel*)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
//#if 1
//        /* Writing in channel mode.*/
//        chnWrite(&PORTAB_SDU1, buf, sizeof buf - 1);
//#else
//        /* Writing in buffer mode.*/
//        (void)obqGetEmptyBufferTimeout(&PORTAB_SDU1.obqueue, TIME_INFINITE);
//        memcpy(PORTAB_SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
//        obqPostFullBuffer(&PORTAB_SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
//#endif
//    }
//    chprintf(chp, "\r\n\nstopped\r\n");
//}

// static const ShellCommand commands[] = {{"write", cmd_write}, {NULL, NULL}};

// static const ShellConfig shell_cfg1 = {(BaseSequentialStream*)&PORTAB_SDU1, commands};

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg)
{

    (void)arg;
    chRegSetThreadName("blinker");
    while(true) {
        systime_t time = serusbcfg.usbp->state == USB_ACTIVE ? 100 : 1000;
        palToggleLine(PORTAB_BLINK_LED1);
        chThdSleepMilliseconds(time);
    }
}

/*
 * Application entry point.
 */
int main()
{

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();
    // Remap to USB pins
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

    /*
     * Initializes a serial-over-USB CDC driver.
     */
    sduObjectInit(&PORTAB_SDU1);
    sduStart(&PORTAB_SDU1, &serusbcfg);

    /*
     * Activates the USB driver and then the USB bus pull-up on D+.
     * Note, a delay is inserted in order to not have to disconnect the cable
     * after a reset.
     */
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepSeconds(1);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    /*
     * Shell manager initialization.
     */
    //    shellInit();

    /*
     * Creates the blinker thread.
     */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /*
     * Normal main() thread activity, spawning shells.
     */
    bool i{};
    while(true) {
        if(PORTAB_SDU1.config->usbp->state == USB_ACTIVE) {
            chnWrite((BaseSequentialStream*)&PORTAB_SDU1, i ? buf0 : buf1, 512);
            //            obqGetEmptyBufferTimeout(&PORTAB_SDU1.obqueue, TIME_INFINITE);
            //            memcpy(PORTAB_SDU1.obqueue.ptr, i ? buf1 : buf0, SERIAL_USB_BUFFERS_SIZE);
            //            obqPostFullBuffer(&PORTAB_SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
            //            thread_t* shelltp =
            //              chThdCreateFromHeap(NULL, SHELL_WA_SIZE, "shell", NORMALPRIO + 1, shellThread,
            //              (void*)&shell_cfg1);
            //            chThdWait(shelltp); /* Waiting termination.             */
            i ^= true;
        }
        chThdSleepMilliseconds(1000);
    }
}
