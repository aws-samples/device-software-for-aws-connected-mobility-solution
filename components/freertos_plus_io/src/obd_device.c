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
 * @file obd_device.h
 * @brief Implementation of functions to access obd device io.
 */

#include <string.h>

#include "FreeRTOS_DriverInterface.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_sntp.h"

#include "obd_device.h"

/*-----------------------------------------------------------*/

#define LINK_UART_BAUDRATE         ( 115200 )
#define LINK_UART_NUM              ( UART_NUM_2 )
#define LINK_UART_BUF_SIZE         ( 256 )
#define PIN_LINK_UART_RX           ( 13 )
#define PIN_LINK_UART_TX           ( 14 )
#define PIN_LINK_RESET             ( 15 )

#define OBD_TIMEOUT_LONG_MS        ( 10000 )
#define DEFAULT_READ_TIMEOUT_MS    ( 1000 )

typedef struct ObdDeviceContext
{
    uint32_t readTimeoutMs;
} ObdDeviceContext_t;

/*-----------------------------------------------------------*/

static Peripheral_Descriptor_t Obd_Open( const int8_t * pcPath,
                                        const uint32_t ulFlags );

static size_t Obd_Write( Peripheral_Descriptor_t const pxPeripheral,
                         const void * pvBuffer,
                         const size_t xBytes );

static size_t Obd_Read( Peripheral_Descriptor_t const pxPeripheral,
                        void * const pvBuffer,
                        const size_t xBytes );

static BaseType_t Obd_Ioctl( Peripheral_Descriptor_t const xPeripheral,
                            uint32_t ulRequest,
                            void * pvValue );

static ObdDeviceContext_t obdDeviceContext =
{
    DEFAULT_READ_TIMEOUT_MS
};

Peripheral_device_t gObdDevice =
{
    "/dev/obd",
    Obd_Open,
    Obd_Write,
    Obd_Read,
    Obd_Ioctl,
    &obdDeviceContext
};

/*-----------------------------------------------------------*/

static unsigned long IRAM_ATTR esp_millis()
{
    return ( unsigned long ) ( esp_timer_get_time() / 1000ULL );
}

/*-----------------------------------------------------------*/

static int uart_receive( char * buffer,
                         int bufsize,
                         unsigned int timeout )
{
    unsigned char n = 0;
    unsigned long startTime = esp_millis();
    unsigned long elapsed;

    for( ; ; )
    {
        elapsed = esp_millis() - startTime;

        if( elapsed > timeout )
        {
            break;
        }

        if( n >= bufsize - 1 )
        {
            break;
        }

        int len = uart_read_bytes( LINK_UART_NUM, ( uint8_t * ) buffer + n, bufsize - n - 1, 1 );

        if( len < 0 )
        {
            break;
        }

        if( len == 0 )
        {
            continue;
        }

        buffer[ n + len ] = 0;

        if( strstr( buffer + n, "\r>" ) )
        {
            n += len;
            break;
        }

        n += len;

        if( strstr( buffer, "..." ) )
        {
            buffer[ 0 ] = 0;
            n = 0;
            timeout += OBD_TIMEOUT_LONG_MS;
        }
    }

    #if VERBOSE_LINK
        printf( "[UART RECV]: %d", buffer );
    #endif
    return n;
}

/*-----------------------------------------------------------*/

static void Obd_Reset( void )
{
    gpio_set_direction( PIN_LINK_RESET, GPIO_MODE_OUTPUT );
    gpio_set_level( PIN_LINK_RESET, 0 );
    vTaskDelay( pdMS_TO_TICKS( 50 ) );
    gpio_set_level( PIN_LINK_RESET, 1 );
    vTaskDelay( pdMS_TO_TICKS( 1000 ) );
}

/*-----------------------------------------------------------*/

static bool get_UTCTime( char *pBuffer, uint32_t bufferLength )
{
    static bool retGetUtcTime = true;
    time_t now = 0;
    struct tm timeinfo = { 0 };

    time( &now );
    localtime_r( &now, &timeinfo );
    strftime( pBuffer, bufferLength, "%Y-%m-%dT%H:%M:%S.0000Z", &timeinfo );

    return retGetUtcTime;
}

/*-----------------------------------------------------------*/

Peripheral_Descriptor_t Obd_Open( const int8_t * pcPath,
                                  const uint32_t ulFlags )
{
    Peripheral_Descriptor_t retDesc = NULL;

    /* Reset the OBD link. */
    Obd_Reset();

    /* Open the UART device. */
    uart_config_t uart_config =
    {
        .baud_rate           = LINK_UART_BAUDRATE,
        .data_bits           = UART_DATA_8_BITS,
        .parity              = UART_PARITY_DISABLE,
        .stop_bits           = UART_STOP_BITS_1,
        .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    /* Configure UART parameters. */
    uart_param_config( LINK_UART_NUM, &uart_config );

    /* Set UART pins. */
    uart_set_pin( LINK_UART_NUM, PIN_LINK_UART_TX, PIN_LINK_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );

    /* Install UART driver. */
    if( uart_driver_install( LINK_UART_NUM, LINK_UART_BUF_SIZE, 0, 0, NULL, 0 ) == ESP_OK )
    {
        retDesc = ( Peripheral_Descriptor_t ) &gObdDevice;
    }
    else
    {
        printf( "Open OBD device fail, delete then retried" );
        uart_driver_delete( LINK_UART_NUM );
        if( uart_driver_install( LINK_UART_NUM, LINK_UART_BUF_SIZE, 0, 0, NULL, 0 ) == ESP_OK )
        {
            retDesc = ( Peripheral_Descriptor_t ) &gObdDevice;
        }
        else
        {
            printf( "Open OBD device retry failed" );
        }
    }

    return retDesc;
}

/*-----------------------------------------------------------*/

size_t Obd_Write( Peripheral_Descriptor_t const pxPeripheral,
                  const void * pvBuffer,
                  const size_t xBytes )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    size_t retSize = 0;

    if( pDevice == NULL )
    {
        printf( "Obd_Write bad param pxPeripheral" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else if( pDevice->pDeviceData == NULL )
    {
        printf( "Obd_Write bad param pDeviceData" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else
    {
        retSize = uart_write_bytes( LINK_UART_NUM, pvBuffer, xBytes );
    }

    return retSize;
}

/*-----------------------------------------------------------*/

size_t Obd_Read( Peripheral_Descriptor_t const pxPeripheral,
                 void * const pvBuffer,
                 const size_t xBytes )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    ObdDeviceContext_t * pObdContext = NULL;
    size_t retSize = 0;
    char * const buffer = ( char * const ) pvBuffer;
    uint32_t bufsize = xBytes;

    if( pDevice == NULL )
    {
        printf( "Obd_Read bad param pxPeripheral" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else if( pDevice->pDeviceData == NULL )
    {
        printf( "Obd_Read bad param pDeviceData" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else
    {
        pObdContext = ( ObdDeviceContext_t * ) pDevice->pDeviceData;
        retSize = uart_receive( buffer, bufsize, pObdContext->readTimeoutMs );
    }

    return retSize;
}

/*-----------------------------------------------------------*/

BaseType_t Obd_Ioctl( Peripheral_Descriptor_t const pxPeripheral,
                      uint32_t ulRequest,
                      void * pvValue )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    ObdDeviceContext_t * pObdContext = NULL;
    BaseType_t retValue = pdPASS;

    /* configPRINTF(("%s:%d\r\n", __FUNCTION__, __LINE__)); */
    if( pDevice == NULL )
    {
        printf( "Obd_Ioctl bad param pxPeripheral" );
        retValue = pdFAIL;
    }
    else if( pDevice->pDeviceData == NULL )
    {
        printf( "Obd_Ioctl bad param pDeviceData" );
        retValue = pdFAIL;
    }
    else
    {
        pObdContext = ( ObdDeviceContext_t * ) pDevice->pDeviceData;

        switch( ulRequest )
        {
            case ioctlOBD_READ_TIMEOUT:

                if( pvValue == NULL )
                {
                    printf( "ioctlOBD_READ_TIMEOUT bad param pvValue" );
                    retValue = pdFAIL;
                }
                else
                {
                    pObdContext->readTimeoutMs = *( ( uint32_t * ) pvValue );
                }

                break;

            case ioctlOBD_RESET:
                Obd_Reset();
                break;

            case ioctlOBD_NTP:
                if( pvValue == NULL )
                {
                    printf( "ioctlOBD_NTP bad param pvValue" );
                    retValue = pdFAIL;
                }
                else
                {
                    /* TODO : remove the hardcoded buffer length. */
                    if( get_UTCTime( pvValue, 64 ) == false )
                    {
                        printf( "ioctlOBD_NTP failed" );
                        retValue = pdFAIL;
                    }
                }
                break;

            default:
                printf( "unsupported ioctl request fail 0x%08x", ulRequest );
                retValue = pdFAIL;
                break;
        }
    }

    return retValue;
}

/*-----------------------------------------------------------*/
