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
 * @file simulated_route.c
 * @brief Implementation of function to simulate vehicle route when GPS is not available.
 */

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "FreeRTOS_IO.h"
#include "cms_log.h"

#include "obd_data.h"
#include "obd_pid.h"
#include "../include/obd_context.h"
#include "../include/obd_config.h"

/*-----------------------------------------------------------*/

static const char *TAG = "gpsRoute";

/*-----------------------------------------------------------*/

void udpateSimulatedGPSData( obdContext_t * pObdContext )
{
    double gpsStep = 0.0;

    const double x1 = OBD_SIMULATED_TRIP_X1;
    const double y1 = OBD_SIMULATED_TRIP_Y1;
    const double x2 = OBD_SIMULATED_TRIP_X2;
    const double y2 = OBD_SIMULATED_TRIP_Y2;

    CMS_LOGI( TAG, "Simulated GPS data." );
    if( ( pObdContext->startLatitude == 0 ) && ( pObdContext->startLongitude == 0 ) )
    {
        pObdContext->startLatitude = x1;
        pObdContext->startLongitude = y1;
    }

    if( pObdContext->obdTelemetryData.vehicle_speed >= 100 )
    {
        gpsStep = 0.0004;
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed >= 50 )
    {
        gpsStep = 0.0002;
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed > 0 )
    {
        gpsStep = 0.0001;
    }
    else
    {
        gpsStep = 0.0;
    }

    if( ( pObdContext->latitude == 0 ) && ( pObdContext->longitude == 0 ) )
    {
        pObdContext->latitude = pObdContext->startLatitude;
        pObdContext->longitude = pObdContext->startLongitude;
    }
    else
    {
        /* Move on direction and change direction/ */
        switch( pObdContext->startDirection )
        {
        case 0:
            pObdContext->longitude = pObdContext->longitude + gpsStep;
            if( pObdContext->longitude > y2 )
            {
                pObdContext->longitude = y2;
                pObdContext->startDirection = 1;
            }
            break;
        case 1:
            pObdContext->latitude = pObdContext->latitude - gpsStep;
            if( pObdContext->latitude < x2 )
            {
                pObdContext->latitude = x2;
                pObdContext->startDirection = 2;
            }
            break;
        case 2:
            pObdContext->longitude = pObdContext->longitude - gpsStep;
            if( pObdContext->longitude < y1 )
            {
                pObdContext->longitude = y1;
                pObdContext->startDirection = 3;
            }
            break;
        case 3:
            pObdContext->latitude = pObdContext->latitude + gpsStep;
            if( pObdContext->latitude > x1 )
            {
                pObdContext->latitude = x1;
                pObdContext->startDirection = 0;
            }
            break;
        default:
            break;
        }
    }
}

/*-----------------------------------------------------------*/
