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
 * @file buzz_library.c
 * @brief Functions to interact with OBD dongle buzzer.
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "FreeRTOS_IO.h"
#include "buzz_device.h"

/*-----------------------------------------------------------*/

#define BUZZ_BEEP_FREQUENCY         ( 2000 )
#define BUZZ_BEEP_INTERVAL_MS       ( 50 )

/*-----------------------------------------------------------*/

void buzz_playtone( Peripheral_Descriptor_t buzzDevice, uint16_t freq, uint32_t durationMs )
{
    FreeRTOS_ioctl( buzzDevice, ioctlBUZZ_SET_FREQUENCY, &freq );
    FreeRTOS_ioctl( buzzDevice, ioctlBUZZ_ON, NULL );
    vTaskDelay( pdMS_TO_TICKS( durationMs ) );
    FreeRTOS_ioctl( buzzDevice, ioctlBUZZ_OFF, NULL );
}

/*-----------------------------------------------------------*/

void buzz_beep( Peripheral_Descriptor_t buzzDevice, uint32_t beepDurationMs, uint32_t times )
{
    uint32_t i = 0;
    uint16_t freq = BUZZ_BEEP_FREQUENCY;
    
    FreeRTOS_ioctl( buzzDevice, ioctlBUZZ_SET_FREQUENCY, &freq );
    
    for( i = 0; i < times; i++ )
    {
        vTaskDelay( pdMS_TO_TICKS( BUZZ_BEEP_INTERVAL_MS ) );
        FreeRTOS_ioctl( buzzDevice, ioctlBUZZ_ON, NULL );
        vTaskDelay( pdMS_TO_TICKS( beepDurationMs ) );
        FreeRTOS_ioctl( buzzDevice, ioctlBUZZ_OFF, NULL );
    }
}

void buzz_init( Peripheral_Descriptor_t buzzDevice )
{
    
}

/*-----------------------------------------------------------*/