/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file FreeRTOS_DriverInterface.h
 * @brief Define driver io interface structure.
 */

#ifndef FREERTOS_DRIVERINTERFACE_H
#define FREERTOS_DRIVERINTERFACE_H

#include "FreeRTOS_IO.h"

#define DEVICE_PATH_MAX    ( 128 )

typedef Peripheral_Descriptor_t ( * Peripheral_open_Function_t )( const int8_t * pcPath,
                                                                  const uint32_t ulFlags );

typedef size_t ( * Peripheral_write_Function_t )( Peripheral_Descriptor_t const pxPeripheral,
                                                  const void * pvBuffer,
                                                  const size_t xBytes );

typedef size_t ( * Peripheral_read_Function_t )( Peripheral_Descriptor_t const pxPeripheral,
                                                 void * const pvBuffer,
                                                 const size_t xBytes );

typedef BaseType_t ( * Peripheral_ioctl_Function_t )( Peripheral_Descriptor_t const pxPeripheral,
                                                      uint32_t ulRequest,
                                                      void * pvValue );

typedef struct Peripheral_device
{
    char pDevicePath[ DEVICE_PATH_MAX ];
    Peripheral_open_Function_t pOpen;
    Peripheral_write_Function_t pWrite;
    Peripheral_read_Function_t pRead;
    Peripheral_ioctl_Function_t pIoctl;
    void * pDeviceData;
} Peripheral_device_t;

#endif /* FREERTOS_DRIVERINTERFACE_H */
