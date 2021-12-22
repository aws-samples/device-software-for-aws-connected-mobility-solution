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
 * @file gps_library.c
 * @brief Functions to interact with OBD dongle gps.
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "FreeRTOS_IO.h"

#include "obd_library.h"
#include "gps_library.h"

/*-----------------------------------------------------------*/

#define GET_UPTIME_MS()         ( (uint32_t) ( xTaskGetTickCount() * portTICK_PERIOD_MS ) )

#define GPS_DEVICE_READY_TIME_MS       ( 1000 )
#define GPS_COMMAND_TIMEOUT_MS         ( 100 )
#define GPS_NMEA_COMMAND_TIMEOUT_MS    ( 200 )

#define abs( x )    ( x > 0.0 ? ( x ) : ( x * -1.0 ) )

/*-----------------------------------------------------------*/

int GPSLib_GetNMEA( Peripheral_Descriptor_t obdDevice,
                    char * buffer,
                    int bufsize )
{
    return OBDLib_SendCommand( obdDevice, "ATGRR\r", buffer, bufsize, GPS_NMEA_COMMAND_TIMEOUT_MS );
}

/*-----------------------------------------------------------*/

bool GPSLib_Begin( Peripheral_Descriptor_t obdDevice )
{
    char buf[ 64 ] = { 0 };
    uint32_t startTime = GET_UPTIME_MS();
    bool retGPSBegin = false;

    OBDLib_SendCommand( obdDevice, "ATGPSON\r", buf, sizeof( buf ), GPS_COMMAND_TIMEOUT_MS );

    do
    {
        /* Wait until get NMEA is okay. */
        if( ( GPSLib_GetNMEA( obdDevice, buf, sizeof( buf ) ) > 0 ) && ( strstr( buf, ( "$G" ) ) != NULL ) )
        {
            retGPSBegin = true;
            break;
        }
    } while ( ( GET_UPTIME_MS() - startTime ) < GPS_DEVICE_READY_TIME_MS );

    return retGPSBegin;
}

/*-----------------------------------------------------------*/

/* Define for strtoi overflow checking. */
#define CONST_INT32_QUOTIENT_10         ( 214748364 )
#define CONST_INT32_REMAINDER_10        ( 7 )

static int prvGpsStrtoi( const char * pStr, int32_t * pResult )
{
    bool retStrtoi = true;
    int32_t result = 0, i = 0, digit = 0, tmpResult = 0;
    int32_t signFlag = 0;

    if( pStr == NULL )
    {
        retStrtoi = false;
    }
    else if( pStr[0] == '-' )
    {
        signFlag = 1;
        i = 1;
    }
    else
    {
        /* Empty Else MISRA 15.7 */
    }

    if( retStrtoi == true )
    {
        for( ; pStr[i] != '\0'; i++ )
        {
            tmpResult = result;
            if( ( pStr[i] >= '0' ) && ( pStr[i] <= '9' ) )
            {
                digit = ( ( ( int32_t ) ( pStr[i] ) ) - ( ( int32_t ) '0' ) );
                result = ( result * 10 ) + digit;
            }
            else
            {
                /* Non digits should break. */
                retStrtoi = true;
                break;
            }

            /* Overflow detection. */
            if( tmpResult > CONST_INT32_QUOTIENT_10 )
            {
                retStrtoi = false;
            }
            else if( ( tmpResult == CONST_INT32_QUOTIENT_10 ) && ( digit > ( CONST_INT32_REMAINDER_10 + signFlag ) ) )
            {
                retStrtoi = false;
            }
            else
            {
                /* Empty Else MISRA 15.7 */
            }

            if( retStrtoi != true )
            {
                break;
            }
        }
    }

    /* Handle the value sign part. */
    if( retStrtoi == true )
    {
        if( signFlag == 1 )
        {
            *pResult = result * ( -1 );
        }
        else
        {
            *pResult = result;
        }
    }

    return retStrtoi;
}

/*-----------------------------------------------------------*/

static int prvGpsAtoi( const char * pStr )
{
    int retValue = 0;

    if( prvGpsStrtoi( pStr, &retValue ) == false)
    {
        retValue = 0;
    }
    return retValue;
}

/*-----------------------------------------------------------*/

bool GPSLib_GetData( Peripheral_Descriptor_t obdDevice,
                     ObdGpsData_t * gpsData )
{
    char buf[ 160 ];

    if( OBDLib_SendCommand( obdDevice, "ATGPS\r", buf, sizeof( buf ), GPS_COMMAND_TIMEOUT_MS ) == 0 )
    {
        return false;
    }

    char * s = strstr( buf, "$GNIFO," );

    if( !s )
    {
        return false;
    }

    s += 7;
    double lat = 0;
    double lng = 0;
    double alt = 0;
    bool good = false;

    do
    {
        uint32_t date = prvGpsAtoi( s );

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        uint32_t time = prvGpsAtoi( ++s );

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        if( !date )
        {
            break;
        }

        gpsData->date = date;
        gpsData->time = time;
        lat = ( double ) prvGpsAtoi( ++s ) / 1000000;

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        lng = ( double ) prvGpsAtoi( ++s ) / 1000000;

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        alt = ( double ) prvGpsAtoi( ++s ) / 100;
        good = true;

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        gpsData->speed = ( double ) prvGpsAtoi( ++s ) / 100;

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        gpsData->heading = prvGpsAtoi( ++s ) / 100;

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        gpsData->sat = prvGpsAtoi( ++s );

        if( !( s = strchr( s, ',' ) ) )
        {
            break;
        }

        gpsData->hdop = prvGpsAtoi( ++s );
    } while( 0 );

    if( good && ( gpsData->lat || gpsData->lng || gpsData->alt ) )
    {
        /* filter out invalid coordinates */
        good = ( abs( lat * 1000000 - gpsData->lat * 1000000 ) < 100000 && abs( lng * 1000000 - gpsData->lng * 1000000 ) < 100000 );
    }

    if( !good )
    {
        return false;
    }

    gpsData->lat = lat;
    gpsData->lng = lng;
    gpsData->alt = alt;
    printf( "[GPSLib] %lf %lf SATS %d Course: %d\r\n",
                gpsData->lat, gpsData->lng, gpsData->sat, gpsData->heading );
    return true;
}

/*-----------------------------------------------------------*/

bool GPSLib_End( Peripheral_Descriptor_t obdDevice )
{
    char buf[ 16 ] = { 0 };
    bool retEnd = false;

    if( obdDevice == NULL )
    {
        retEnd = false;
    }
    else
    {
        if( OBDLib_SendCommand( obdDevice, "ATGPSOFF", buf, sizeof( buf ), 0 ) > 0 )
        {
            retEnd = true;
        }
    }

    return retEnd;
}

/*-----------------------------------------------------------*/