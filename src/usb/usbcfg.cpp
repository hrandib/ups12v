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

#include "usbcfg.h"
#include "usb_helpers.h"

/* Virtual serial port over USB.*/
SerialUSBDriver SDU1;

/*
 * Endpoints to be used for USBD1.
 */
#define USB1_DATA_REQUEST_EP 1
#define USB1_DATA_AVAILABLE_EP 1
#define USB1_INTERRUPT_REQUEST_EP 2

/*
 * USB Device Descriptor.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
  USB_DESC_DEVICE(0x0110, /* bcdUSB (1.1).                    */
                  0x02,   /* bDeviceClass (CDC).              */
                  0x00,   /* bDeviceSubClass.                 */
                  0x00,   /* bDeviceProtocol.                 */
                  0x40,   /* bMaxPacketSize.                  */
                  0x0483, /* idVendor (ST).                   */
                  0x5740, /* idProduct.                       */
                  0x0200, /* bcdDevice.                       */
                  1,      /* iManufacturer.                   */
                  2,      /* iProduct.                        */
                  3,      /* iSerialNumber.                   */
                  1)      /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {sizeof vcom_device_descriptor_data, vcom_device_descriptor_data};

/* Configuration Descriptor tree for a CDC.*/
static const uint8_t vcom_configuration_descriptor_data[67] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(67,   /* wTotalLength.                    */
                         0x02, /* bNumInterfaces.                  */
                         0x01, /* bConfigurationValue.             */
                         0,    /* iConfiguration.                  */
                         0xC0, /* bmAttributes (self powered).     */
                         50),  /* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE(0x00, /* bInterfaceNumber.                */
                     0x00, /* bAlternateSetting.               */
                     0x01, /* bNumEndpoints.                   */
                     0x02, /* bInterfaceClass (Communications
                              Interface Class, CDC section
                              4.2).                            */
                     0x02, /* bInterfaceSubClass (Abstract
                            Control Model, CDC section 4.3).   */
                     0x01, /* bInterfaceProtocol (AT commands,
                              CDC section 4.4).                */
                     0),   /* iInterface.                      */
  /* Header Functional Descriptor (CDC section 5.2.3).*/
  USB_DESC_BYTE(5),     /* bLength.                         */
  USB_DESC_BYTE(0x24),  /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x00),  /* bDescriptorSubtype (Header
                           Functional Descriptor.           */
  USB_DESC_BCD(0x0110), /* bcdCDC.                          */
  /* Call Management Functional Descriptor. */
  USB_DESC_BYTE(5),    /* bFunctionLength.                 */
  USB_DESC_BYTE(0x24), /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x01), /* bDescriptorSubtype (Call Management
                          Functional Descriptor).          */
  USB_DESC_BYTE(0x00), /* bmCapabilities (D0+D1).          */
  USB_DESC_BYTE(0x01), /* bDataInterface.                  */
  /* ACM Functional Descriptor.*/
  USB_DESC_BYTE(4),    /* bFunctionLength.                 */
  USB_DESC_BYTE(0x24), /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x02), /* bDescriptorSubtype (Abstract
                          Control Management Descriptor).  */
  USB_DESC_BYTE(0x02), /* bmCapabilities.                  */
  /* Union Functional Descriptor.*/
  USB_DESC_BYTE(5),    /* bFunctionLength.                 */
  USB_DESC_BYTE(0x24), /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x06), /* bDescriptorSubtype (Union
                          Functional Descriptor).          */
  USB_DESC_BYTE(0x00), /* bMasterInterface (Communication
                          Class Interface).                */
  USB_DESC_BYTE(0x01), /* bSlaveInterface0 (Data Class
                          Interface).                      */
  /* Endpoint 2 Descriptor.*/
  USB_DESC_ENDPOINT(USB1_INTERRUPT_REQUEST_EP | 0x80,
                    0x03,   /* bmAttributes (Interrupt).        */
                    0x0008, /* wMaxPacketSize.                  */
                    0xFF),  /* bInterval.                       */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE(0x01,  /* bInterfaceNumber.                */
                     0x00,  /* bAlternateSetting.               */
                     0x02,  /* bNumEndpoints.                   */
                     0x0A,  /* bInterfaceClass (Data Class
                               Interface, CDC section 4.5).     */
                     0x00,  /* bInterfaceSubClass (CDC section
                               4.6).                            */
                     0x00,  /* bInterfaceProtocol (CDC section
                               4.7).                            */
                     0x00), /* iInterface.                      */
  /* Endpoint 3 Descriptor.*/
  USB_DESC_ENDPOINT(USB1_DATA_AVAILABLE_EP, /* bEndpointAddress.*/
                    0x02,                   /* bmAttributes (Bulk).             */
                    0x0040,                 /* wMaxPacketSize.                  */
                    0x00),                  /* bInterval.                       */
  /* Endpoint 1 Descriptor.*/
  USB_DESC_ENDPOINT(USB1_DATA_REQUEST_EP | 0x80, /* bEndpointAddress.*/
                    0x02,                        /* bmAttributes (Bulk).             */
                    0x0040,                      /* wMaxPacketSize.                  */
                    0x00)                        /* bInterval.                       */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {sizeof vcom_configuration_descriptor_data,
                                                            vcom_configuration_descriptor_data};

/*
 * U.S. English language identifier.
 */
const auto vcom_string0 = 0x0409_sdesc16;

/*
 * Vendor string.
 */
const auto vcom_string1 = "STMicroelectronics"_sdesc;

/*
 * Device Description string.
 */
const auto vcom_string2 = "ChibiOS/RT Virtual COM Port"_sdesc;

/*
 * Serial Number string.
 */
const auto vcom_string3 = CH_KERNEL_VERSION ""_sdesc;

/*
 * Strings wrappers array.
 */
static const USBDescriptor vcom_strings[] = {{vcom_string1.size(), vcom_string0.data()},
                                             {vcom_string1.size(), vcom_string1.data()},
                                             {vcom_string2.size(), vcom_string2.data()},
                                             {vcom_string3.size(), vcom_string3.data()}};

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor* get_descriptor(USBDriver* usbp, uint8_t dtype, uint8_t dindex, uint16_t lang)
{

    (void)usbp;
    (void)lang;
    switch(dtype) {
        case USB_DESCRIPTOR_DEVICE:
            return &vcom_device_descriptor;
        case USB_DESCRIPTOR_CONFIGURATION:
            return &vcom_configuration_descriptor;
        case USB_DESCRIPTOR_STRING:
            if(dindex < 4) {
                return &vcom_strings[dindex];
            }
    }
    return nullptr;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {USB_EP_MODE_TYPE_BULK,
                                            nullptr,
                                            sduDataTransmitted,
                                            sduDataReceived,
                                            0x0040,
                                            0x0040,
                                            &ep1instate,
                                            &ep1outstate,
                                            2,
                                            nullptr};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config =
  {USB_EP_MODE_TYPE_INTR, nullptr, sduInterruptTransmitted, nullptr, 0x0010, 0x0000, &ep2instate, nullptr, 1, nullptr};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver* usbp, usbevent_t event)
{
    switch(event) {
        case USB_EVENT_ADDRESS:
            return;
        case USB_EVENT_CONFIGURED:
            chSysLockFromISR();

            /* Enables the endpoints specified into the configuration.
               Note, this callback is invoked from an ISR so I-Class functions
               must be used.*/
            usbInitEndpointI(usbp, USB1_DATA_REQUEST_EP, &ep1config);
            usbInitEndpointI(usbp, USB1_INTERRUPT_REQUEST_EP, &ep2config);

            /* Resetting the state of the CDC subsystem.*/
            sduConfigureHookI(&SDU1);

            chSysUnlockFromISR();
            return;
        case USB_EVENT_RESET:
            /* Falls into.*/
        case USB_EVENT_UNCONFIGURED:
            /* Falls into.*/
        case USB_EVENT_SUSPEND:
            chSysLockFromISR();

            /* Disconnection event on suspend.*/
            sduSuspendHookI(&SDU1);

            chSysUnlockFromISR();
            return;
        case USB_EVENT_WAKEUP:
            chSysLockFromISR();

            /* Connection event on wakeup.*/
            sduWakeupHookI(&SDU1);

            chSysUnlockFromISR();
            return;
        case USB_EVENT_STALLED:
            return;
    }
}

/*
 * Handles the USB driver global events.
 */
static void sof_handler(USBDriver* usbp)
{

    (void)usbp;

    osalSysLockFromISR();
    sduSOFHookI(&SDU1);
    osalSysUnlockFromISR();
}

/*
 * USB driver configuration.
 */
const USBConfig usbcfg = {usb_event, get_descriptor, sduRequestsHook, sof_handler};

/*
 * Serial over USB driver configuration.
 */
const SerialUSBConfig serusbcfg = {&USBD1, USB1_DATA_REQUEST_EP, USB1_DATA_AVAILABLE_EP, USB1_INTERRUPT_REQUEST_EP};
