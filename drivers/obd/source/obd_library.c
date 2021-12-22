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
 * @file obd_library.c
 * @brief Implementation of functions to interact with obd dongle.
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "FreeRTOS_IO.h"

#include "obd_data.h"
#include "obd_device.h"

#include "obd_pid.h"

#define OBD_TIMEOUT_SHORT_MS        ( 1000 )
#define OBD_TIMEOUT_LONG_MS         ( 10000 )
#define OBD_READ_PID_DELAY_MS       ( 20 )
#define OBD_GET_VIN_DELAY_MS        ( 100 )

/*-----------------------------------------------------------*/

static const uint32_t dataMode = 1;

/*-----------------------------------------------------------*/

static uint8_t checkErrorMessage( const char * buffer )
{
    const char * errmsg[] = { "UNABLE", "ERROR", "TIMEOUT", "NO DATA" };
    uint8_t i = 0;

    for( i = 0; i < sizeof( errmsg ) / sizeof( errmsg[ 0 ] ); i++ )
    {
        if( strstr( buffer, errmsg[ i ] ) )
        {
            return i + 1;
        }
    }

    return 0;
}

/*-----------------------------------------------------------*/

static uint8_t hex2uint8( const char * p )
{
    uint8_t c1 = *p;
    uint8_t c2 = *( p + 1 );

    if( ( c1 >= 'A' ) && ( c1 <= 'F' ) )
    {
        c1 -= 7;
    }
    else if( ( c1 >= 'a' ) && ( c1 <= 'f' ) )
    {
        c1 -= 39;
    }
    else if( ( c1 < '0' ) || ( c1 > '9' ) )
    {
        return 0;
    }

    if( c2 == 0 )
    {
        return( c1 & 0xf );
    }
    else if( ( c2 >= 'A' ) && ( c2 <= 'F' ) )
    {
        c2 -= 7;
    }
    else if( ( c2 >= 'a' ) && ( c2 <= 'f' ) )
    {
        c2 -= 39;
    }
    else if( ( c2 < '0' ) || ( c2 > '9' ) )
    {
        return 0;
    }

    return c1 << 4 | ( c2 & 0xf );
}

/*-----------------------------------------------------------*/

static uint16_t hex2uint16( const char * p )
{
    char c = *p;
    uint16_t i = 0;

    for( uint8_t n = 0; c && n < 4; c = *( ++p ) )
    {
        if( ( c >= 'A' ) && ( c <= 'F' ) )
        {
            c -= 7;
        }
        else if( ( c >= 'a' ) && ( c <= 'f' ) )
        {
            c -= 39;
        }
        else if( ( c == ' ' ) && ( n == 2 ) )
        {
            continue;
        }
        else if( ( c < '0' ) || ( c > '9' ) )
        {
            break;
        }

        i = ( i << 4 ) | ( c & 0xF );
        n++;
    }

    return i;
}

/*-----------------------------------------------------------*/

static uint8_t getPercentageValue( char * data )
{
    return ( uint16_t ) hex2uint8( data ) * 100 / 255;
}

/*-----------------------------------------------------------*/

static uint16_t getLargeValue( char * data )
{
    return hex2uint16( data );
}

/*-----------------------------------------------------------*/

static uint8_t getSmallValue( char * data )
{
    return hex2uint8( data );
}

/*-----------------------------------------------------------*/

static int16_t getTemperatureValue( char * data )
{
    return ( int ) hex2uint8( data ) - 40;
}

/*-----------------------------------------------------------*/

static int normalizeData( uint8_t pid, char * data )
{
    int result;

    switch( pid )
    {
        case PID_RPM:
        case PID_EVAP_SYS_VAPOR_PRESSURE: /* kPa */
            result = getLargeValue( data ) >> 2;
            break;

        case PID_FUEL_PRESSURE: /* kPa */
            result = getSmallValue( data ) * 3;
            break;

        case PID_COOLANT_TEMP:
        case PID_INTAKE_TEMP:
        case PID_AMBIENT_TEMP:
        case PID_ENGINE_OIL_TEMP:
            result = getTemperatureValue( data );
            break;

        case PID_THROTTLE:
        case PID_COMMANDED_EGR:
        case PID_COMMANDED_EVAPORATIVE_PURGE:
        case PID_FUEL_LEVEL:
        case PID_RELATIVE_THROTTLE_POS:
        case PID_ABSOLUTE_THROTTLE_POS_B:
        case PID_ABSOLUTE_THROTTLE_POS_C:
        case PID_ACC_PEDAL_POS_D:
        case PID_ACC_PEDAL_POS_E:
        case PID_ACC_PEDAL_POS_F:
        case PID_COMMANDED_THROTTLE_ACTUATOR:
        case PID_ENGINE_LOAD:
        case PID_ABSOLUTE_ENGINE_LOAD:
        case PID_ETHANOL_FUEL:
        case PID_HYBRID_BATTERY_PERCENTAGE:
            result = getPercentageValue( data );
            break;

        case PID_MAF_FLOW: /* grams/sec */
            result = getLargeValue( data ) / 100;
            break;

        case PID_TIMING_ADVANCE:
            result = ( int ) ( getSmallValue( data ) / 2 ) - 64;
            break;

        case PID_DISTANCE:                 /* km */
        case PID_DISTANCE_WITH_MIL:        /* km */
        case PID_TIME_WITH_MIL:            /* minute */
        case PID_TIME_SINCE_CODES_CLEARED: /* minute */
        case PID_RUNTIME:                  /* second */
        case PID_FUEL_RAIL_PRESSURE:       /* kPa */
        case PID_ENGINE_REF_TORQUE:        /* Nm */
            result = getLargeValue( data );
            break;

        case PID_CONTROL_MODULE_VOLTAGE: /* V */
            result = getLargeValue( data ) / 1000;
            break;

        case PID_ENGINE_FUEL_RATE: /* L/h */
            result = getLargeValue( data ) / 20;
            break;

        case PID_ENGINE_TORQUE_DEMANDED:   /* % */
        case PID_ENGINE_TORQUE_PERCENTAGE: /* % */
            result = ( int ) getSmallValue( data ) - 125;
            break;

        case PID_SHORT_TERM_FUEL_TRIM_1:
        case PID_LONG_TERM_FUEL_TRIM_1:
        case PID_SHORT_TERM_FUEL_TRIM_2:
        case PID_LONG_TERM_FUEL_TRIM_2:
        case PID_EGR_ERROR:
            result = ( ( int ) getSmallValue( data ) - 128 ) * 100 / 128;
            break;

        case PID_FUEL_INJECTION_TIMING:
            result = ( ( int32_t ) getLargeValue( data ) - 26880 ) / 128;
            break;

        case PID_CATALYST_TEMP_B1S1:
        case PID_CATALYST_TEMP_B2S1:
        case PID_CATALYST_TEMP_B1S2:
        case PID_CATALYST_TEMP_B2S2:
            result = getLargeValue( data ) / 10 - 40;
            break;

        case PID_AIR_FUEL_EQUIV_RATIO: /* 0~200 */
            result = ( long ) getLargeValue( data ) * 200 / 65536;
            break;

        default:
            result = getSmallValue( data );
    }

    return result;
}

/*-----------------------------------------------------------*/

size_t OBDLib_SendCommand( Peripheral_Descriptor_t obdDevice,
                           const char * pCmd,
                           char * pBuf,
                           uint32_t bufSize,
                           uint32_t readTimeout )
{
    size_t retSendCommand = 0;

    if( obdDevice == NULL )
    {
        printf( "OBDLib_SendCommand device is invalid\r\n" );
        retSendCommand = 0;
    }
    else if( ( pCmd == NULL ) || ( strlen( pCmd ) == 0 ) )
    {
        printf( "OBDLib_SendCommand command is invalid\r\n" );
        retSendCommand = 0;
    }
    else if( ( pBuf == NULL ) || ( bufSize == 0 ) )
    {
        printf( "OBDLib_SendCommand buffer is invalid\r\n" );
        retSendCommand = 0;
    }
    else
    {
        /* Write the commadn. */
#ifdef OBD_DEBUG
        printf("OBD send cmd %s\r\n", pCmd );
#endif
        retSendCommand = FreeRTOS_write( obdDevice, pCmd, strlen( pCmd ) );

        /* Set the read timeout. */
        if( retSendCommand > 0 )
        {
            if( pdFAIL == FreeRTOS_ioctl( obdDevice, ioctlOBD_READ_TIMEOUT, &readTimeout ) )
            {
                retSendCommand = 0;
            }
        }

        /* Read the response. */
        if( retSendCommand > 0 )
        {
            retSendCommand = FreeRTOS_read( obdDevice, pBuf, bufSize );
#ifdef OBD_DEBUG
            if( retSendCommand > 0 )
            {
                printf("OBD receive buffer %s\r\n", pBuf );
            }
#endif
        }
    }

    return retSendCommand;
}

/*-----------------------------------------------------------*/

bool ODBLib_GetVIN( Peripheral_Descriptor_t obdDevice,
                    char * buffer,
                    uint8_t bufsize )
{
    uint8_t n = 0;
    char vinProcessBuffer[ 128 ] = { 0 };

    for( n = 0; n < 2; n++ )
    {
        if( OBDLib_SendCommand( obdDevice, "0902\r", vinProcessBuffer, sizeof( vinProcessBuffer ), OBD_TIMEOUT_LONG_MS ) )
        {
            int len = hex2uint16( vinProcessBuffer );
            char * p = strstr( vinProcessBuffer + 4, "0: 49 02 01" );

            if( p )
            {
                char * q = buffer;
                p += 11; /* skip the header */

                do
                {
                    while( *( ++p ) == ' ' )
                    {
                    }

                    for( ; ; )
                    {
                        *( q++ ) = hex2uint8( p );

                        while( *p && *p != ' ' )
                        {
                            p++;
                        }

                        while( *p == ' ' )
                        {
                            p++;
                        }

                        if( !*p || ( *p == '\r' ) )
                        {
                            break;
                        }
                    }

                    p = strchr( p, ':' );
                } while( p );

                *q = 0;

                if( q - buffer == len - 3 )
                {
                    return true;
                }
            }
        }

        /* AT least one tick delay. */
        vTaskDelay( pdMS_TO_TICKS(OBD_GET_VIN_DELAY_MS) );
    }

    return false;
}

/*-----------------------------------------------------------*/

int OBDLib_ReadDTC( Peripheral_Descriptor_t obdDevice,
                    uint16_t codes[],
                    uint8_t maxCodes )
{
    /*
     * Response example:
     * 0: 43 04 01 08 01 09
     * 1: 01 11 01 15 00 00 00
     */
    int codesRead = 0;

    for( int n = 0; n < 6; n++ )
    {
        char buffer[ 128 ];
        sprintf( buffer, n == 0 ? "03\r" : "03%02X\r", n );

        if( OBDLib_SendCommand( obdDevice, buffer, buffer, sizeof( buffer ), OBD_TIMEOUT_LONG_MS ) > 0 )
        {
            if( !strstr( buffer, "NO DATA" ) )
            {
                char * p = strstr( buffer, "43" );

                if( p )
                {
                    while( codesRead < maxCodes && *p )
                    {
                        p += 6;

                        if( *p == '\r' )
                        {
                            p = strchr( p, ':' );

                            if( !p )
                            {
                                break;
                            }

                            p += 2;
                        }

                        uint16_t code = hex2uint16( p );

                        if( code == 0 )
                        {
                            break;
                        }

                        codes[ codesRead++ ] = code;
                    }
                }

                break;
            }
        }
    }

    return codesRead;
}

/*-----------------------------------------------------------*/

void OBDLib_ClearDTC( Peripheral_Descriptor_t obdDevice )
{
    char buffer[ 32 ];

    OBDLib_SendCommand( obdDevice, "04\r", buffer, sizeof( buffer ), OBD_TIMEOUT_LONG_MS );
}

/*-----------------------------------------------------------*/

bool OBDLib_ReadPID( Peripheral_Descriptor_t obdDevice,
                     uint8_t pid,
                     int * pResult )
{
    char buffer[ 64 ];
    char * data = 0;
    size_t retSendCommand = 0;

    sprintf( buffer, "%02X%02X\r", dataMode, pid );
    FreeRTOS_write( obdDevice, buffer, strlen( buffer ) ); /* link->send(buffer); */
#ifdef OBD_DEBUG
        printf("OBD ReadPID send cmd %s\r\n", buffer );
#endif

    /* At least one tick delay. */
    vTaskDelay( pdMS_TO_TICKS(OBD_READ_PID_DELAY_MS) );                      /* idleTasks(); */

    retSendCommand = FreeRTOS_read( obdDevice, buffer, 64 );
#ifdef OBD_DEBUG
    printf("OBD ReadPID result %s\r\n", buffer );
#endif

    /* int ret = link->receive(buffer, sizeof(buffer), OBD_TIMEOUT_SHORT_MS); */
    if( ( retSendCommand > 0 ) && !checkErrorMessage( buffer ) )
    {
        char * p = buffer;

        while( ( p = strstr( p, "41 " ) ) )
        {
            p += 3;
            uint8_t curpid = hex2uint8( p );

            if( curpid == pid )
            {
                while( *p && *p != ' ' )
                {
                    p++;
                }

                while( *p == ' ' )
                {
                    p++;
                }

                if( *p )
                {
                    data = p;
                    break;
                }
            }
        }
    }

    if( data == NULL )
    {
        printf("OBD ReadPID result failed %s\r\n", buffer );
        return false;
    }

    *pResult = normalizeData( pid, data );

#ifdef OBD_DEBUG
        printf("OBD ReadPID result %d\r\n", *pResult );
#endif
    return true;
}

/*-----------------------------------------------------------*/

bool OBDLib_ReadUTCTime( Peripheral_Descriptor_t obdDevice,
                        char *pUTCStr, uint32_t bufferSize )
{
    bool retValue = true;
    if( pdFAIL == FreeRTOS_ioctl( obdDevice, ioctlOBD_NTP, pUTCStr ) )
    {
       retValue = false;
    }
    return retValue;
}

/*-----------------------------------------------------------*/

int OBDLib_Init( Peripheral_Descriptor_t obdDevice )
{
    const char * initcmd[] = { "ATE0\r", "ATH0\r" };
    char buffer[ 64 ];
    uint32_t n = 0, i = 0;
    uint32_t stage = 0;
    int value;
    uint8_t pidmap[ 4 * 8 ] = { 0 };
    char * p = NULL;
    //size_t retReadSize = 0;

    /* Softreset. */
    for( n = 0; n < 10; n++ )
    {
        if( OBDLib_SendCommand( obdDevice, "ATZ\r", buffer, sizeof( buffer ), OBD_TIMEOUT_SHORT_MS ) > 0 )
        {
            stage = 1;
            break;
        }
    }

    /* Sent init command. */
    if( stage == 1 )
    {
        for( i = 0; i < sizeof( initcmd ) / sizeof( initcmd[ 0 ] ); i++ )
        {
            OBDLib_SendCommand( obdDevice, initcmd[ i ], buffer, sizeof( buffer ), OBD_TIMEOUT_SHORT_MS );
        }

        stage = 2;
    }
    else
    {
        return -1;
    }

    /* Read SPEED for testing. */
    if( stage == 2 )
    {
        for( n = 0; n < 5; n++ )
        {
            if( OBDLib_ReadPID( obdDevice, PID_SPEED, &value ) )
            {
                stage = 3;
                break;
            }
        }
    }
    else
    {
        return -2;
    }

    /* Read the PID map. */
    if( stage == 3 )
    {
        memset( pidmap, 0xff, sizeof( pidmap ) );

        for( i = 0; i < 8; i++ )
        {
            uint8_t pid = i * 0x20;
            sprintf( buffer, "%02X%02X\r", dataMode, pid );
            FreeRTOS_write( obdDevice, buffer, strlen( buffer ) );

            //retReadSize = FreeRTOS_read( obdDevice, buffer, sizeof( buffer ) );
            if( checkErrorMessage( buffer ) )
            {
                continue;
            }

            for( p = buffer; ( p = strstr( p, "41 " ) ); )
            {
                p += 3;

                if( hex2uint8( p ) == pid )
                {
                    p += 2;

                    for( n = 0; n < 4 && *( p + n * 3 ) == ' '; n++ )
                    {
                        pidmap[ i * 4 + n ] = hex2uint8( p + n * 3 + 1 );
                    }
                }
            }
        }
    }
    else
    {
        return -3;
    }

    return 0;
}

/*-----------------------------------------------------------*/
