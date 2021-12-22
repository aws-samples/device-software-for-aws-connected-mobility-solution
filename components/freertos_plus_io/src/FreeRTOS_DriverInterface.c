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
 * @file FreeRTOS_DriverInterface.c
 * @brief implementation of driver io interface.
 */

#include <string.h>

#include "FreeRTOS_IO.h"
#include "FreeRTOS_DriverInterface.h"

#include "obd_device.h"
#include "buzz_device.h"

/*-----------------------------------------------------------*/

/* Define the ioctl of OBD devices. */
extern Peripheral_device_t gObdDevice;

/* Define the ioctl of BUZZ devices. */
extern Peripheral_device_t gBuzzDevice;

/* Define the secure information. */
extern Peripheral_device_t gSecureDevice;

/*-----------------------------------------------------------*/

static Peripheral_device_t * pPeripheralDevices[] =
{
    &gObdDevice,
    &gBuzzDevice,
    &gSecureDevice
};

/*-----------------------------------------------------------*/

Peripheral_Descriptor_t FreeRTOS_open( const int8_t * pcPath,
                                       const uint32_t ulFlags )
{
    uint32_t i = 0;
    uint32_t deviceListSize = sizeof( pPeripheralDevices ) / sizeof( Peripheral_device_t * );
    Peripheral_device_t * pPeripheralDevice = NULL;

    for( i = 0; i < deviceListSize; i++ )
    {
        if( strcmp( pPeripheralDevices[ i ]->pDevicePath, ( const char * ) pcPath ) == 0 )
        {
            pPeripheralDevice = pPeripheralDevices[ i ];
            break;
        }
    }

    /* Call the open function. */
    if( ( pPeripheralDevice != NULL ) && ( pPeripheralDevice->pOpen != NULL ) )
    {
        pPeripheralDevice = ( Peripheral_device_t * ) pPeripheralDevice->pOpen( pcPath, ulFlags );
    }

    return ( Peripheral_Descriptor_t ) pPeripheralDevice;
}

/*-----------------------------------------------------------*/

size_t FreeRTOS_read( Peripheral_Descriptor_t const pxPeripheral,
                      void * const pvBuffer,
                      const size_t xBytes )
{
    Peripheral_device_t * pPeripheralDevice = NULL;
    size_t retRead = 0;

    if( pxPeripheral != NULL )
    {
        pPeripheralDevice = ( Peripheral_device_t * ) pxPeripheral;

        if( pPeripheralDevice->pRead != NULL )
        {
            retRead = pPeripheralDevice->pRead( pPeripheralDevice, pvBuffer, xBytes );
        }
    }

    return retRead;
}

/*-----------------------------------------------------------*/

size_t FreeRTOS_write( Peripheral_Descriptor_t const pxPeripheral,
                       const void * pvBuffer,
                       const size_t xBytes )
{
    Peripheral_device_t * pPeripheralDevice = NULL;
    size_t retWrite = 0;

    if( pxPeripheral != NULL )
    {
        pPeripheralDevice = ( Peripheral_device_t * ) pxPeripheral;

        if( pPeripheralDevice->pWrite != NULL )
        {
            retWrite = pPeripheralDevice->pWrite( pPeripheralDevice, pvBuffer, xBytes );
        }
    }

    return retWrite;
}

/*-----------------------------------------------------------*/

BaseType_t FreeRTOS_ioctl( Peripheral_Descriptor_t const pxPeripheral,
                           uint32_t ulRequest,
                           void * pvValue )
{
    Peripheral_device_t * pPeripheralDevice = NULL;
    size_t retIoctl = 0;

    if( pxPeripheral != NULL )
    {
        pPeripheralDevice = ( Peripheral_device_t * ) pxPeripheral;

        if( pPeripheralDevice->pIoctl != NULL )
        {
            retIoctl = pPeripheralDevice->pIoctl( pPeripheralDevice, ulRequest, pvValue );
        }
    }

    return retIoctl;
}

/*-----------------------------------------------------------*/
