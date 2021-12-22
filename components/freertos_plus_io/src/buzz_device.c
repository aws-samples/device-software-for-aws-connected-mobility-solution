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
 * @file buzz_device.c
 * @brief Implementation of functions to access buzz device io.
 */

#include <string.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IO.h"
#include "FreeRTOS_DriverInterface.h"

#include "buzz_device.h"
#include "driver/ledc.h"

/*-----------------------------------------------------------*/

#define PIN_BUZZER                  ( 25 )
#define BUZZ_OUTPUT_SPEED           ( LEDC_HIGH_SPEED_MODE )
#define BUZZ_DEFAULT_FREQ           ( 2000 )
#define BUZZ_MAX_DUTY               ( 0x0080 )
#define BUZZ_DEFAULT_DUTY_PERCENT   ( 100 )
#define BUZZ_TIMER                  ( LEDC_TIMER_0 )
#define BUZZ_LEDC_CHANNEL           ( LEDC_CHANNEL_0 )
#define BUZZ_DUTY( x )              ( ( BUZZ_MAX_DUTY * ( x ) ) / 100 )

typedef struct BuzzContext
{
    ledc_timer_config_t timer_conf;
    ledc_channel_config_t ledc_conf;
    uint16_t duty;
    uint16_t freq;
} BuzzContext_t;

/*-----------------------------------------------------------*/

static Peripheral_Descriptor_t Buzz_Open( const int8_t * pcPath,
                                          const uint32_t ulFlags );

static BaseType_t Buzz_Ioctl( Peripheral_Descriptor_t const xPeripheral,
                              uint32_t ulRequest,
                              void * pvValue );

static BuzzContext_t gBuzzContext = { 0 };

Peripheral_device_t gBuzzDevice =
{
    "/dev/buzz",
    Buzz_Open,
    NULL, /* Empty Write function. */
    NULL, /* Empty Read function. */
    Buzz_Ioctl,
    &gBuzzContext
};

/*-----------------------------------------------------------*/

Peripheral_Descriptor_t Buzz_Open( const int8_t * pcPath,
                                   const uint32_t ulFlags )
{
    Peripheral_Descriptor_t retDesc = NULL;

    printf( "%s:%d\r\n", __FUNCTION__, __LINE__ );
    retDesc = ( Peripheral_Descriptor_t ) &gBuzzDevice;

    gBuzzContext.duty = BUZZ_DEFAULT_DUTY_PERCENT;
    gBuzzContext.freq = BUZZ_DEFAULT_FREQ;

    gBuzzContext.timer_conf.speed_mode = BUZZ_OUTPUT_SPEED;
    // gBuzzContext.timer_conf.bit_num = LEDC_TIMER_10_BIT;
    gBuzzContext.timer_conf.timer_num = BUZZ_TIMER;
    gBuzzContext.timer_conf.duty_resolution = LEDC_TIMER_10_BIT;
    gBuzzContext.timer_conf.freq_hz = gBuzzContext.freq;
    ledc_timer_config( &gBuzzContext.timer_conf );

    gBuzzContext.ledc_conf.gpio_num = PIN_BUZZER;
    gBuzzContext.ledc_conf.speed_mode = BUZZ_OUTPUT_SPEED;
    gBuzzContext.ledc_conf.channel = BUZZ_LEDC_CHANNEL;
    gBuzzContext.ledc_conf.intr_type = LEDC_INTR_DISABLE;
    gBuzzContext.ledc_conf.timer_sel = BUZZ_TIMER;
    gBuzzContext.ledc_conf.duty = BUZZ_DUTY( 0 ); /* 100% for 10 bits. */

    ledc_channel_config( &gBuzzContext.ledc_conf );

    return retDesc;
}

/*-----------------------------------------------------------*/

BaseType_t Buzz_Ioctl( Peripheral_Descriptor_t const pxPeripheral,
                       uint32_t ulRequest,
                       void * pvValue )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    BaseType_t retValue = pdPASS;
    BuzzContext_t * pBuzzContext = NULL;

    if( pDevice == NULL )
    {
        printf( "Buzz_Ioctl bad param pxPeripheral\r\n" );
        retValue = pdFAIL;
    }
    else
    {
        pBuzzContext = ( BuzzContext_t * ) pDevice->pDeviceData;

        switch( ulRequest )
        {
            case ioctlBUZZ_ON:
                ledc_set_freq( BUZZ_OUTPUT_SPEED, BUZZ_TIMER, pBuzzContext->freq );
                ledc_set_duty( BUZZ_OUTPUT_SPEED, BUZZ_LEDC_CHANNEL, BUZZ_DUTY( pBuzzContext->duty ) );
                ledc_update_duty( BUZZ_OUTPUT_SPEED, BUZZ_LEDC_CHANNEL );
                break;

            case ioctlBUZZ_OFF:
                ledc_set_duty( BUZZ_OUTPUT_SPEED, BUZZ_LEDC_CHANNEL, 0 );
                ledc_update_duty( BUZZ_OUTPUT_SPEED, BUZZ_LEDC_CHANNEL );
                break;

            case ioctlBUZZ_SET_FREQUENCY:
                if( pvValue == NULL )
                {
                    retValue = pdFAIL;
                }
                else
                {
                    pBuzzContext->freq = * ( uint16_t * )pvValue;
                }
                break;

            case ioctlBUZZ_SET_DUTY:
                if( pvValue == NULL )
                {
                    retValue = pdFAIL;
                }
                else
                {
                    pBuzzContext->duty = * ( uint16_t * )pvValue;
                }
                break;

            default:
                break;
        }
    }

    return retValue;
}

/*-----------------------------------------------------------*/
