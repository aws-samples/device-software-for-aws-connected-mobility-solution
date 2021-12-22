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
 * @file obd_main.c
 * @brief Implementation of obd task and feature functions.
 */

#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "core_mqtt_agent_tasks.h"

#include "obd_data.h"
#include "FreeRTOS_IO.h"

#include "obd_pid.h"

#include "obd_library.h"
#include "gps_library.h"
#include "buzz_library.h"
#include "secure_device.h"

#include "../include/obd_context.h"
#include "../include/obd_config.h"

// log print header
#include "cms_log.h"

/*-----------------------------------------------------------*/

#define OBD_AGGREGATED_DATA_INTERVAL_STEPS      ( OBD_AGGREGATED_DATA_INTERVAL_MS / OBD_DATA_COLLECT_INTERVAL_MS )
#define OBD_TELEMETRY_DATA_INTERVAL_STEPS       ( OBD_TELEMETRY_DATA_INTERVAL_MS / OBD_DATA_COLLECT_INTERVAL_MS )
#define OBD_SIMULATED_TRIP_STEPS                ( OBD_SIMULATED_TRIP_MS / OBD_TELEMETRY_DATA_INTERVAL_MS )

#define xTaskGetTickCountMs()                   ( uint32_t ) ( xTaskGetTickCount() * portTICK_PERIOD_MS )

#define TIME_SELECTION_NONE                     ( 0 )
#define TIME_SELECTION_GPS                      ( 1 )
#define TIME_SELECTION_NTP                      ( 2 )
#define TIME_SELECTION_UPTIME                   ( 3 )

#define MAX_DTC_CODES                           ( 6U )
#define MAX_RETRY_TIMES                         ( 3U )
#define OBD_DEFAULT_VIN                         "chingleeVin1\0"

#ifndef OBD_TELEMETRY_TYPE_OIL_TEMP_PID
    #define OBD_TELEMETRY_TYPE_OIL_TEMP_PID     PID_ENGINE_OIL_TEMP
#endif

#define OBD_MQTT_QOS                            MQTTQoS1

/*-----------------------------------------------------------*/

static const char *TAG = "vehicleTelemetry";

static obdContext_t gObdContext =
{
    .obdAggregatedData           = { 0 },
    .obdTelemetryData            = { 0 },
    .thingName                   = "ThingNameDefault",
    .tripId                      = "123",
    .vin                         = "WASM_test_car",
    .telemetryIndex              = 0,
    .latitude                    = 0.0,
    .longitude                   = 0.0,
    .startLatitude               = 0.0,
    .startLongitude              = 0.0,
    .transmission_gear_position  = "neutral",
    .isoTime                     = "1970-01-01T00:00:00.0000Z",
    .timeSelection               = TIME_SELECTION_NONE,
    .lastUpdateTicksMs           = 0,
    .updateCount                 = 0,
    .highSpeedDurationMs         = 0,
    .idleSpeedDurationMs         = 0,
    .idleSpeedDurationIntervalMs = 0,
    .higRpmDurationIntervalMs    = 0,
    .obdDevice                   = NULL,
    .buzzDevice                  = NULL,
    .gpsReceived                 = false,
    .obdDeviceConnected          = false
};

static const char OBD_DATA_TRIP_TOPIC[] = "dt/cvra/%s/trip";
static const char OBD_DATA_TRIP_FORMAT_1[] =
"{ \r\n\
    \"MessageId\": \"%s\", \r\n\
    \"CreationTimeStamp\": \"%s\", \r\n\
    \"SendTimeStamp\": \"%s\", \r\n\
    \"VIN\": \"%s\", \r\n\
    \"TripId\": \"%s\", \r\n\
";
static const char OBD_DATA_TRIP_FORMAT_2[] =
"    \"TripSummary\": { \r\n\
        \"StartTime\": \"%s\", \r\n";

static const char OBD_DATA_TRIP_FORMAT_3[] =
"        \"Distance\": %lf, \r\n\
        \"Duration\": %u, \r\n\
        \"Fuel\": %lf, \r\n\
";
static const char OBD_DATA_TRIP_FORMAT_4[] =
"        \"StartLocation\": { \r\n\
            \"Latitude\": %lf, \r\n\
            \"Longitude\": %lf, \r\n\
            \"Altitude\": %lf \r\n\
        }, \r\n\
";
static const char OBD_DATA_TRIP_FORMAT_5[] =
"        \"EndLocation\": { \r\n\
            \"Latitude\": %lf, \r\n\
            \"Longitude\": %lf, \r\n\
            \"Altitude\": %lf \r\n\
        }, \r\n\
        \"SpeedProfile\": %lf \r\n\
    } \r\n\
}";

static const char OBD_DATA_TELEMETRY_TOPIC[] = "dt/cvra/%s/cardata";
/* The value field is vary from the field. Use strcat to cat the string. */
static const char OBD_DATA_TELEMETRY_FORMAT_1[] =
"{ \r\n\
    \"MessageId\": \"%s\", \r\n\
    \"SimulationId\": \"%s\", \r\n\
    \"CreationTimeStamp\": \"%s\", \r\n\
    \"SendTimeStamp\": \"%s\", \r\n\
    \"VIN\": \"%s\", \r\n\
    \"TripId\": \"%s\", \r\n\
    \"DriverID\": \"%s\", \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_2[] =
"    \"GeoLocation\": { \r\n\
        \"Latitude\": %lf, \r\n\
        \"Longitude\": %lf, \r\n\
        \"Altitude\": %lf, \r\n\
        \"Heading\": %lf, \r\n\
        \"Speed\": %lf \r\n\
    }, \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_3[] =
"    \"Communications\": { \r\n\
        \"GSM\": { \r\n\
            \"Satelites\": \"%s\", \r\n\
            \"Fix\": \"%s\", \r\n\
            \"NetworkType\": \"%s\", \r\n\
            \"MNC\": \"%s\", \r\n\
            \"MCC\": \"%s\", \r\n\
            \"LAC\": \"%s\", \r\n\
            \"CID\": \"%s\" \r\n\
        }, \r\n\
        \"WiFi\": { \r\n\
            \"NetworkID\": \"%s\" \r\n\
        }, \r\n\
        \"Wired\": { \r\n\
            \"NetworkID\": \"%s\" \r\n\
        } \r\n\
    }, \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_4[] =
"    \"Acceleration\": { \r\n\
        \"MaxLongitudinal\": { \r\n\
            \"Axis\": %lf, \r\n\
            \"Value\": %lf \r\n\
        }, \r\n\
        \"MaxLateral\": { \r\n\
            \"Axis\": %lf, \r\n\
            \"Value\": %lf \r\n\
        } \r\n\
    }, \r\n\
    \"Throttle\": { \r\n\
        \"Max\": %lf, \r\n\
        \"Average\": %lf \r\n\
    }, \r\n\
    \"Speed\": { \r\n\
        \"Max\": %lf, \r\n\
        \"Average\": %lf \r\n\
    }, \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_5[] =
"    \"Odometer\": { \r\n\
        \"Metres\": %lf, \r\n\
        \"TicksFL\": %lf, \r\n\
        \"TicksFR\": %lf, \r\n\
        \"TicksRL\": %lf, \r\n\
        \"TicksRR\": %lf \r\n\
    }, \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_6[] =
"    \"Fuel\": %lf, \r\n\
    \"Name\": \"%s\", \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_7[] =
"    \"OilTemp\": %lf, \r\n\
    \"FuelInfo\": { \r\n\
        \"CurrentTripConsumption\": %lf, \r\n\
        \"TankCapacity\": %lf \r\n\
    }, \r\n\
";

static const char OBD_DATA_TELEMETRY_FORMAT_8[] =
"    \"IgnitionStatus\": \"%s\" \r\n\
}";

static const char OBD_DATA_DTC_TOPIC[] = "dt/cvra/%s/dtc";
static const char OBD_DATA_DTC_FORMAT[] =
"{ \r\n\
    \"MessageId\": \"%s\", \r\n\
    \"CreationTimeStamp\": \"%s\", \r\n\
    \"SendTimeStamp\": \"%s\", \r\n\
    \"VIN\": \"%s\", \r\n\
    \"DTC\": { \r\n\
        \"Code\": \"P%04x\", \r\n\
        \"Changed\": \"%s\" \r\n\
    } \r\n\
}";

static const char OBD_MAINTENANCE_TOPIC[] = "dt/cvra/%s/maintenance";
static const char OBD_MAINTENANCE_FORMAT[] =
"{ \r\n\
    \"MessageId\": \"%s\", \r\n\
    \"CreationTimeStamp\": \"%s\", \r\n\
    \"SendTimeStamp\": \"%s\", \r\n\
    \"VIN\": \"%s\", \r\n\
    \"Maintenance\": \r\n\
    { \r\n\
        \"Id\": \"%s\", \r\n\
        \"Val\": \"%s\" \r\n\
    } \r\n\
}";

/*-----------------------------------------------------------*/

extern UBaseType_t uxRand( void );
extern void udpateSimulatedGPSData( obdContext_t * pObdContext );

/*-----------------------------------------------------------*/

static void genTripId( obdContext_t * pObdContext )
{
    ObdGpsData_t gpsData = { 0 };
    uint32_t randomBuf = 0;
    bool retGetData = false;

    /* retIoctl = FreeRTOS_ioctl( gObdContext.obdDevice, ioctlOBD_GPS_READ, &gpsData ); */
    retGetData = GPSLib_GetData( gObdContext.obdDevice, &gpsData );

    if( retGetData == true )
    {
        // char * p = pObdContext->tripId + sprintf( pObdContext->tripId, 
        //                                           "%04u%02u%02u%02u%02u%02u",
        //                                           ( unsigned int ) ( gpsData.date % 100 ) + 2000, 
        //                                           ( unsigned int ) ( gpsData.date / 100 ) % 100, 
        //                                           ( unsigned int ) ( gpsData.date / 10000 ),
        //                                           ( unsigned int ) ( gpsData.time / 1000000 ), 
        //                                           ( unsigned int ) ( gpsData.time % 1000000 ) / 10000, 
        //                                           ( unsigned int ) ( gpsData.time % 10000 ) / 100 );
        // *p = '\0';
        snprintf( pObdContext->tripId, 
                  "%04u%02u%02u%02u%02u%02u",
                  sizeof(pObdContext->tripId),
                  ( unsigned int ) ( gpsData.date % 100 ) + 2000, 
                  ( unsigned int ) ( gpsData.date / 100 ) % 100, 
                  ( unsigned int ) ( gpsData.date / 10000 ),
                  ( unsigned int ) ( gpsData.time / 1000000 ), 
                  ( unsigned int ) ( gpsData.time % 1000000 ) / 10000, 
                  ( unsigned int ) ( gpsData.time % 10000 ) / 100 );
    }
    else
    {
        /* Random generated trip ID. */
        randomBuf = uxRand();
        snprintf( pObdContext->tripId, OBD_TRIP_ID_MAX, "%08u%d",
                  randomBuf, xTaskGetTickCountMs() );
    }
    snprintf( pObdContext->tripName, OBD_TRIP_NAME_MAX, "trip_%s", pObdContext->tripId );
}

/*-----------------------------------------------------------*/

static void covertTicksToTimeFormat( uint64_t ticksMs,
                                     char * pTime,
                                     uint32_t bufferLength )
{
    /* Use gettickcount as time stamp from 1970-01-01 00:00:00.0000. */
    uint64_t upTimeMs = 0;
    uint64_t temp = 0;
    uint32_t days = 1;
    uint32_t hours = 0;
    uint32_t minutes = 0;
    uint32_t seconds = 0;
    uint32_t remainMs = 0;

    upTimeMs = ticksMs;
    temp = upTimeMs / ( uint64_t ) ( 24 * 60 * 60 * 1000 );
    days = ( temp <= UINT32_MAX ) ? ( uint32_t ) temp : 0;

    upTimeMs = upTimeMs - days * ( uint64_t ) ( 24 * 60 * 60 * 1000 );
    temp = upTimeMs / ( uint64_t ) ( 60 * 60 * 1000 );
    hours = ( temp <= UINT32_MAX ) ? ( uint32_t ) temp : 0;
    
    upTimeMs = upTimeMs - hours * ( uint64_t ) ( 60 * 60 * 1000 );
    temp = upTimeMs / ( uint64_t ) ( 60 * 1000 );
    minutes = ( temp <= UINT32_MAX ) ? ( uint32_t ) temp : 0;

    upTimeMs = upTimeMs - minutes * ( uint64_t ) ( 60 * 1000 );
    temp = upTimeMs / ( uint64_t ) ( 1000 );
    seconds = ( temp <= UINT32_MAX ) ? ( uint32_t ) temp : 0;

    upTimeMs = upTimeMs - seconds * ( uint64_t ) ( 1000 );
    remainMs = ( temp <= UINT32_MAX ) ? ( uint32_t ) temp : 0;

    snprintf( pTime, bufferLength, "%04u-%02u-%02uT%02u:%02u:%02u.%04uZ",
              ( uint32_t ) 1970,
              ( uint32_t ) 1,
              ( uint32_t ) ( days + 1 ),
              ( uint32_t ) hours,
              ( uint32_t ) minutes,
              ( uint32_t ) seconds,
              ( uint32_t ) remainMs );
}

/*-----------------------------------------------------------*/

static void updateTimestamp( obdContext_t * pObdContext,
                             ObdGpsData_t * pGpsData )
{
    /* Time source selection. */
    if( pObdContext->timeSelection == TIME_SELECTION_NONE )
    {
        if( pGpsData != NULL )
        {
            CMS_LOGI( TAG, "Timestamp source GPS." );
            pObdContext->timeSelection = TIME_SELECTION_GPS;
        }
        else if( OBDLib_ReadUTCTime( pObdContext->obdDevice, pObdContext->isoTime, OBD_ISO_TIME_MAX ) == true )
        {
            CMS_LOGI( TAG, "Timestamp source NTP." );
            pObdContext->timeSelection = TIME_SELECTION_NTP;
        }
        else
        {
            CMS_LOGI( TAG, "Timestamp source UPTIME." );
            pObdContext->timeSelection = TIME_SELECTION_UPTIME;
        }
    }

    /* If we can't get GPS data, we use the uptime. */
    if( pObdContext->timeSelection == TIME_SELECTION_GPS )
    {
        if( pGpsData != NULL )
        {
            float kph = ( float ) ( ( int ) ( pGpsData->speed * 1.852f * 10 ) ) / 10;

            pObdContext->timeSelection = TIME_SELECTION_GPS;
            char * p = pObdContext->isoTime + snprintf( pObdContext->isoTime, OBD_ISO_TIME_MAX, "%04u-%02u-%02uT%02u:%02u:%02u",
                                                        ( unsigned int ) ( pGpsData->date % 100 ) + 2000,
                                                        ( unsigned int ) ( pGpsData->date / 100 ) % 100,
                                                        ( unsigned int ) ( pGpsData->date / 10000 ),
                                                        ( unsigned int ) ( pGpsData->time / 1000000 ),
                                                        ( unsigned int ) ( pGpsData->time % 1000000 ) / 10000,
                                                        ( unsigned int ) ( pGpsData->time % 10000 ) / 100 );
            unsigned char tenth = ( pGpsData->time % 100 ) / 10;

            if( tenth )
            {
                p += sprintf( p, ".%c000", '0' + tenth );
            }

            *p = 'Z';
            *( p+1 ) = '\0';

            CMS_LOGD( TAG, "[GPS] %lf %lf %lf km/h SATS %d Course: %d %s.",
                            pGpsData->lat, pGpsData->lng, kph, pGpsData->sat, pGpsData->heading, pObdContext->isoTime );
        }
    }
    else if( pObdContext->timeSelection == TIME_SELECTION_NTP )
    {
        OBDLib_ReadUTCTime( pObdContext->obdDevice, pObdContext->isoTime, OBD_ISO_TIME_MAX );
    }
    else if( pObdContext->timeSelection == TIME_SELECTION_UPTIME )
    {
        pObdContext->timeSelection = TIME_SELECTION_UPTIME;
        covertTicksToTimeFormat( ( uint64_t ) xTaskGetTickCountMs(), pObdContext->isoTime, OBD_ISO_TIME_MAX );
    }
    else
    {
        CMS_LOGE( TAG, "Unable to update time with source %u.", pObdContext->timeSelection );
    }
}

/*-----------------------------------------------------------*/

static void genSimulateGearPosition( obdContext_t * pObdContext )
{
    if( pObdContext->obdTelemetryData.vehicle_speed == 0 )
    {
        strncpy( pObdContext->transmission_gear_position, "neutral", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed < 30 )
    {
        strncpy( pObdContext->transmission_gear_position, "first", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed < 50 )
    {
        strncpy( pObdContext->transmission_gear_position, "second", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed < 70 )
    {
        strncpy( pObdContext->transmission_gear_position, "third", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed < 90 )
    {
        strncpy( pObdContext->transmission_gear_position, "fourth", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
    else if( pObdContext->obdTelemetryData.vehicle_speed < 110 )
    {
        strncpy( pObdContext->transmission_gear_position, "fifth", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
    else
    {
        strncpy( pObdContext->transmission_gear_position, "sixth", OBD_TRANSMISSION_GEAR_POSITION_MAX );
    }
}

/*-----------------------------------------------------------*/

static void genSimulatePadelPosition( obdContext_t * pObdContext )
{
    if( pObdContext->obdTelemetryData.engine_speed <= 0 )
    {
        pObdContext->obdTelemetryData.accelerator_pedal_position = 0;
    }
    else if( ( pObdContext->obdTelemetryData.engine_speed > 0 ) &&
             ( pObdContext->obdTelemetryData.engine_speed <= CAR_ACCELARATOR_PADEL_RPM_THRESHOLD ) )
    {
        pObdContext->obdTelemetryData.accelerator_pedal_position =
            ( pObdContext->obdTelemetryData.engine_speed * 100.0 ) / CAR_ACCELARATOR_PADEL_RPM_THRESHOLD;
    }
    else
    {
        pObdContext->obdTelemetryData.accelerator_pedal_position = 100;
    }

    pObdContext->obdAggregatedData.accelerator_pedal_position_mean =
        ( pObdContext->obdAggregatedData.accelerator_pedal_position_mean * ( pObdContext->updateCount ) ) +
        pObdContext->obdTelemetryData.accelerator_pedal_position;
    pObdContext->obdAggregatedData.accelerator_pedal_position_mean =
        pObdContext->obdAggregatedData.accelerator_pedal_position_mean / ( pObdContext->updateCount + 1 );
}

/*-----------------------------------------------------------*/

static void resetTelemetryData( obdContext_t * pObdContext )
{
    memset( &pObdContext->obdAggregatedData, 0, sizeof( ObdAggregatedData_t ) );
    memset( &pObdContext->obdTelemetryData, 0, sizeof( ObdTelemetryData_t ) );
    pObdContext->telemetryIndex = 0;
    pObdContext->latitude = 0;
    pObdContext->longitude = 0;
    pObdContext->startLatitude = 0;
    pObdContext->startLongitude = 0;
    pObdContext->timeSelection = TIME_SELECTION_NONE;
    pObdContext->lastUpdateTicksMs = 0;
    pObdContext->updateCount = 0;
    pObdContext->highSpeedDurationMs = 0;
    pObdContext->idleSpeedDurationMs = 0;
    pObdContext->idleSpeedDurationIntervalMs = 0;
    pObdContext->higRpmDurationIntervalMs = 0;
    pObdContext->fuel_consumed_since_restart = 0.0;
}

/*-----------------------------------------------------------*/

static double obdReadVehicleSpeed( obdContext_t * pObdContext )
{
    BaseType_t retIoctl = pdPASS;
    int32_t pidValue = 0;
    double vehicleSpeed = 0;

    if( pObdContext->obdDeviceConnected == true )
    {
        retIoctl = OBDLib_ReadPID( pObdContext->obdDevice, PID_SPEED, &pidValue );
    }
    else
    {
        pidValue = OBD_SIMULATED_VEHICLE_SPEED;
    }

    if( retIoctl == pdPASS )
    {
        vehicleSpeed = ( double ) pidValue;
    }

    return vehicleSpeed;
}

/*-----------------------------------------------------------*/

static double updateGPSData( obdContext_t * pObdContext, bool useSimulatledGPSData )
{
    ObdGpsData_t gpsData = { 0 };
    bool retGetData = false;
    double kph = 0.0;

    /* Update GPS data. */
    if( useSimulatledGPSData == false )
    {
        retGetData = GPSLib_GetData( pObdContext->obdDevice, &gpsData );

        if( retGetData == true )
        {
            if( pObdContext->gpsReceived == false )
            {
                pObdContext->gpsReceived = true;
                buzz_beep( pObdContext->buzzDevice, BUZZ_SHORT_BEEP_DURATION_MS, 3 );
            }

            CMS_LOGD( TAG, "GPS good data." );
            pObdContext->latitude = gpsData.lat;
            pObdContext->longitude = gpsData.lng;

            if( ( pObdContext->startLatitude == 0 ) && ( pObdContext->startLongitude == 0 ) &&
                ( ( gpsData.lat != 0 ) || ( gpsData.lng != 0 ) ) )
            {
                pObdContext->startLatitude = gpsData.lat;
                pObdContext->startLongitude = gpsData.lng;
            }

            kph = ( double ) ( ( int ) ( gpsData.speed * 1.852f * 10 ) ) / 10;
        }
    }
    else
    {
        udpateSimulatedGPSData( pObdContext );
    }

    return kph;
}

/*-----------------------------------------------------------*/

static void updateTelemetryData( obdContext_t * pObdContext )
{
    int32_t pidValue = 0;
    ObdTelemetryDataType_t pidIndex = OBD_TELEMETRY_TYPE_STEERING_WHEEL_ANGLE;
    uint64_t currentTicksMs = ( uint64_t ) xTaskGetTickCountMs();
    uint64_t timeDiffMs = currentTicksMs - pObdContext->lastUpdateTicksMs;

    bool retReadPID = true;
    
    /* If OBD not connected, we use the simulated data. */
    if( gObdContext.obdDeviceConnected == false )
    {
        pObdContext->obdTelemetryData.vehicle_speed = OBD_SIMULATED_VEHICLE_SPEED;
        pObdContext->updateCount = pObdContext->updateCount + 1;
        return;
    }

    /* Update telemetry and aggregated data. */
    for( pidIndex = OBD_TELEMETRY_TYPE_STEERING_WHEEL_ANGLE; pidIndex < OBD_TELEMETRY_TYPE_MAX; pidIndex++ )
    {
        switch( pidIndex )
        {
            case OBD_TELEMETRY_TYPE_OIL_TEMP:
                retReadPID = OBDLib_ReadPID( pObdContext->obdDevice, OBD_TELEMETRY_TYPE_OIL_TEMP_PID, &pidValue );

                if( retReadPID == true )
                {
                    pObdContext->obdTelemetryData.oil_temp = ( double ) pidValue;
                    /* Convert to Fahrenheit. */
                    pObdContext->obdTelemetryData.oil_temp = pObdContext->obdTelemetryData.oil_temp * 1.8 + 32.0;
                    if( pObdContext->obdAggregatedData.oil_temp_mean == 0 )
                    {
                        pObdContext->obdAggregatedData.oil_temp_mean = pObdContext->obdTelemetryData.oil_temp;
                    }
                    else
                    {
                        pObdContext->obdAggregatedData.oil_temp_mean =
                            ( pObdContext->obdAggregatedData.oil_temp_mean * ( pObdContext->updateCount ) ) +
                            pObdContext->obdTelemetryData.oil_temp;
                        pObdContext->obdAggregatedData.oil_temp_mean =
                            pObdContext->obdAggregatedData.oil_temp_mean / ( pObdContext->updateCount + 1 );
                    }
                }
                
                if( pObdContext->higRpmDurationIntervalMs > CAR_HIGH_OIL_TEMP_RPM_DURATION_MS )
                {
                    CMS_LOGI( TAG, "CAR high oil temp." );
                    pObdContext->obdTelemetryData.oil_temp = CAR_HIGH_OIL_TEMP;
                }

                break;

            case OBD_TELEMETRY_TYPE_ENGINE_SPEED:
                retReadPID = OBDLib_ReadPID( pObdContext->obdDevice, PID_RPM, &pidValue );

                if( retReadPID == true )
                {
                    pObdContext->obdTelemetryData.engine_speed = ( double ) pidValue;

                    if( pObdContext->obdAggregatedData.engine_speed_mean == 0 )
                    {
                        pObdContext->obdAggregatedData.engine_speed_mean = pObdContext->obdTelemetryData.engine_speed;
                    }
                    else
                    {
                        pObdContext->obdAggregatedData.engine_speed_mean =
                            ( pObdContext->obdAggregatedData.engine_speed_mean * ( pObdContext->updateCount ) ) +
                            pObdContext->obdTelemetryData.engine_speed;
                        pObdContext->obdAggregatedData.engine_speed_mean =
                            pObdContext->obdAggregatedData.engine_speed_mean / ( pObdContext->updateCount + 1 );
                    }

                    /* Update the simulated high rpm oil temperature. */
                    if( pObdContext->obdTelemetryData.engine_speed > ( ( double ) CAR_HIGH_OIL_TEMP_RPM ) )
                    {
                        pObdContext->higRpmDurationIntervalMs = pObdContext->higRpmDurationIntervalMs + timeDiffMs;
                    }
                    else
                    {
                        pObdContext->higRpmDurationIntervalMs = 0;
                    }

                    /* Update the simulated padel position. */
                    genSimulatePadelPosition( pObdContext );
                }

                break;

            case OBD_TELEMETRY_TYPE_VEHICLE_SPEED:
                retReadPID = OBDLib_ReadPID( pObdContext->obdDevice, PID_SPEED, &pidValue );

                if( retReadPID == true )
                {
                    /* Update the simulated Acceleration. */
                    double previous_speed = pObdContext->obdTelemetryData.vehicle_speed;
                    pObdContext->obdTelemetryData.vehicle_speed = ( double ) pidValue;

                    if( timeDiffMs != 0 )
                    {
                        pObdContext->obdTelemetryData.acceleration =
                            ( pObdContext->obdTelemetryData.vehicle_speed - previous_speed ) * ( 1000 ) / timeDiffMs;
                    }

                    /* CMS_LOGD( TAG, "Acceleration %lf.", pObdContext->obdTelemetryData.acceleration ); */

                    /* Update the simulated high speed duration. */
                    if( pObdContext->obdTelemetryData.vehicle_speed > ( ( double ) CAR_HIGH_SPEED_THRESHOLD ) )
                    {
                        pObdContext->highSpeedDurationMs = pObdContext->highSpeedDurationMs + timeDiffMs;
                    }

                    /* Update the idle time. */
                    if( pObdContext->obdTelemetryData.vehicle_speed <= ( ( double ) CAR_IDLE_SPEED_THRESHOLD ) )
                    {
                        pObdContext->idleSpeedDurationMs = pObdContext->idleSpeedDurationMs + timeDiffMs;
                        pObdContext->idleSpeedDurationIntervalMs = pObdContext->idleSpeedDurationIntervalMs + timeDiffMs;
                    }
                    else
                    {
                        pObdContext->idleSpeedDurationIntervalMs = 0;
                    }

                    /* Update the simulated gear position. */
                    genSimulateGearPosition( pObdContext );

                    /* Update the aggregated vehicle speed mean. */
                    if( pObdContext->obdAggregatedData.vehicle_speed_mean == 0 )
                    {
                        pObdContext->obdAggregatedData.vehicle_speed_mean = pObdContext->obdTelemetryData.vehicle_speed;
                    }
                    else
                    {
                        pObdContext->obdAggregatedData.vehicle_speed_mean =
                            ( pObdContext->obdAggregatedData.vehicle_speed_mean * ( pObdContext->updateCount ) ) +
                            pObdContext->obdTelemetryData.vehicle_speed;
                        pObdContext->obdAggregatedData.vehicle_speed_mean =
                            pObdContext->obdAggregatedData.vehicle_speed_mean / ( pObdContext->updateCount + 1 );
                    }
                }

                break;

            /*
             * case OBD_TELEMETRY_TYPE_TORQUE_AT_TRANSMISSION:
             *  retIoctl = OBDLib_ReadPID( pObdContext->obdDevice, PID_ENGINE_TORQUE_PERCENTAGE, &pidValue );
             *  if( retIoctl == pdPASS )
             *  {
             *      pObdContext->obdTelemetryData.torque_at_transmission = ( double )pidValue;
             *  }
             *  break;
             */
            case OBD_TELEMETRY_TYPE_FUEL_LEVEL:
                retReadPID = OBDLib_ReadPID( pObdContext->obdDevice, PID_FUEL_LEVEL, &pidValue );

                if( retReadPID == true )
                {
                    pObdContext->fuel_level = ( double ) pidValue / 100.0;
                    /* Update the simulated fuel_consumed_since_restart. */
                    pObdContext->fuel_consumed_since_restart =
                        ( pObdContext->start_fuel_level - pObdContext->fuel_level ) * CAR_GAS_TANK_SIZE;
                }

                break;

            case OBD_TELEMETRY_TYPE_ODOMETER:
                /* Update simulated data. */
                pObdContext->odometer = pObdContext->obdAggregatedData.vehicle_speed_mean *
                                        ( currentTicksMs - pObdContext->startTicksMs ) / ( 60 * 60 * 1000 );
                break;

            default:
                break;
        }
    }

    /* Update ticks. */
    pObdContext->lastUpdateTicksMs = ( uint64_t ) xTaskGetTickCountMs();
    pObdContext->updateCount = pObdContext->updateCount + 1;
}

/*-----------------------------------------------------------*/

static BaseType_t checkObdDtcData( obdContext_t * pObdContext )
{
    uint32_t i = 0;
    uint16_t dtc[ MAX_DTC_CODES ];
    int retCodeRead = 0;
    char messageId[ OBD_MESSAGE_ID_MAX ] = { 0 };
    BaseType_t retMqtt = pdPASS;

    snprintf( messageId, OBD_MESSAGE_ID_MAX, "%s-%s", pObdContext->vin, pObdContext->isoTime );

    if( pObdContext->obdDeviceConnected == true )
    {
        retCodeRead = OBDLib_ReadDTC( pObdContext->obdDevice, dtc, MAX_DTC_CODES );
    }

    for( i = 0; i < retCodeRead; i++ )
    {
        snprintf( pObdContext->topicBuf, OBD_TOPIC_BUF_SIZE, OBD_DATA_DTC_TOPIC, pObdContext->thingName );
        snprintf( pObdContext->messageBuf, OBD_MESSAGE_BUF_SIZE, OBD_DATA_DTC_FORMAT,
                  messageId,
                  pObdContext->isoTime,     // CreationTimeStamp
                  pObdContext->isoTime,     // SendTimeStamp
                  pObdContext->vin,         // vin
                  dtc[ i ],                 // DTC code
                  "true"                    // changed alwasy true
                  );

        retMqtt = mqttAgentPublish( OBD_MQTT_QOS,
                                    pObdContext->topicBuf,
                                    strlen( pObdContext->topicBuf ),
                                    pObdContext->messageBuf,
                                    strlen( pObdContext->messageBuf ) );

        OBDLib_ClearDTC( pObdContext->obdDevice );
    }


    return retMqtt;
}

/*-----------------------------------------------------------*/

static BaseType_t sendObdTelemetryData( obdContext_t * pObdContext )
{
    BaseType_t retMqtt = pdPASS;
    char messageId[ OBD_MESSAGE_ID_MAX ] = { 0 };
    uint32_t msgLength = 0;
    
    snprintf( messageId, OBD_MESSAGE_ID_MAX, "%s-%s", pObdContext->vin, pObdContext->isoTime );

    snprintf( pObdContext->topicBuf, OBD_TOPIC_BUF_SIZE, OBD_DATA_TELEMETRY_TOPIC, pObdContext->thingName );

    msgLength = snprintf( pObdContext->messageBuf,
            OBD_MESSAGE_BUF_SIZE, OBD_DATA_TELEMETRY_FORMAT_1,
              messageId,                // messageId
              "iotlabtpesim",           // SimulationId
              pObdContext->isoTime,     // CreationTimeStamp
              pObdContext->isoTime,     // SendTimeStamp
              pObdContext->vin,         // vin
              pObdContext->tripId,      // TripId
              ""                        // DriverID
    );

    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
            OBD_MESSAGE_BUF_SIZE, OBD_DATA_TELEMETRY_FORMAT_2,
              // GeoLocation data
              pObdContext->latitude,    // Latitude
              pObdContext->longitude,    // Longitude
              0.0,                      // Altitude
              0.0,                      // Heading
              pObdContext->obdTelemetryData.vehicle_speed      // Speed KM/H
    );
    
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
            OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TELEMETRY_FORMAT_3,
              // Communications
              "",                       // Satelites
              "",                       // Fix
              "",                       // NetworkType
              "",                       // MNC
              "",                       // MCC
              "",                       // LAC
              "",                       // CID
              // WIFI
              "",                       // NetworkID
              // Wired
              ""                       // NetworkID
    );
    
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
            OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TELEMETRY_FORMAT_4,
              // Acceleration
              // MaxLongitudinal
              0.0,                      // Axis
              0.0,                      // Value
              // MaxLateral
              0.0,                      // Axis
              0.0,                      // Value
              // Throttle
              0.0,                      // Max
              0.0,                      // Average
              // Speed
              pObdContext->obdTelemetryData.vehicle_speed,      // Max
              pObdContext->obdAggregatedData.vehicle_speed_mean     // Average
    );
    
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ],
            OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TELEMETRY_FORMAT_5,
              // Odometer
              pObdContext->odometer,    // metres KM can be 0
              0.0,                      // TicksFL
              0.0,                      // TicksFR
              0.0,                      // TicksRL
              0.0                       // TicksRR
    );
    
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ],
            OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TELEMETRY_FORMAT_6,
              pObdContext->fuel_level * CAR_GAS_TANK_SIZE,  // Fuel in L
              pObdContext->tripName                         // Name unique route name for this trip
    );
    
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ],
            OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TELEMETRY_FORMAT_7,
              pObdContext->obdTelemetryData.oil_temp,       // OilTemp

              // FuelInfo
              pObdContext->fuel_consumed_since_restart,     // CurrentTripConsumption
              CAR_GAS_TANK_SIZE                            // TankCapacity
    );
    
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ],
            OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TELEMETRY_FORMAT_8,
            pObdContext->ignition_status                  // IgnitionStatus
    );

    retMqtt = mqttAgentPublish( OBD_MQTT_QOS,
                                pObdContext->topicBuf,
                                strlen( pObdContext->topicBuf ),
                                pObdContext->messageBuf,
                                strlen( pObdContext->messageBuf )
                              );
    return retMqtt;
}

/*-----------------------------------------------------------*/

static BaseType_t sendObdTripData( obdContext_t * pObdContext )
{
    BaseType_t retMqtt = pdPASS;
    char messageId[ OBD_MESSAGE_ID_MAX ] = { 0 };
    uint32_t msgLength = 0;
    uint32_t tripDuration = ( uint32_t ) ( pObdContext->lastUpdateTicksMs - pObdContext->startTicksMs );

    snprintf( messageId, OBD_MESSAGE_ID_MAX, "%s-%s", pObdContext->vin, pObdContext->isoTime );
    
    snprintf( pObdContext->topicBuf, OBD_TOPIC_BUF_SIZE, OBD_DATA_TRIP_TOPIC, pObdContext->thingName );
    msgLength = snprintf( pObdContext->messageBuf, OBD_MESSAGE_BUF_SIZE, OBD_DATA_TRIP_FORMAT_1,
              messageId,                // MessageId
              pObdContext->isoTime,     // CreationTimeStamp
              pObdContext->isoTime,     // SendTimeStamp
              pObdContext->vin,         // vin
              pObdContext->tripId       // TripId
    );
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
                OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TRIP_FORMAT_2,
              // TripSummary
              pObdContext->obdAggregatedData.start_time        // StartTime
    );

    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
                OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TRIP_FORMAT_3,
              pObdContext->odometer,                            // Distance
              tripDuration,                                     // Duration in milliseconds
              pObdContext->fuel_consumed_since_restart * 1000.0  // Fuel in ml
    );
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
                OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TRIP_FORMAT_4,
              // StartLocation
              pObdContext->startLatitude,   // Latitude
              pObdContext->startLongitude,  // Longitude
              0.0                          // Altitude
    );
    msgLength = msgLength + snprintf( &pObdContext->messageBuf[ msgLength ], 
                OBD_MESSAGE_BUF_SIZE - msgLength, OBD_DATA_TRIP_FORMAT_5,
              // EndLocation
              pObdContext->latitude,        // Latitude
              pObdContext->longitude,       // Longitude
              0.0,                          // Altitude
              0.0                           // SpeedProfiler
              );

    retMqtt = mqttAgentPublish( OBD_MQTT_QOS,
                                pObdContext->topicBuf,
                                strlen( pObdContext->topicBuf ),
                                pObdContext->messageBuf,
                                strlen( pObdContext->messageBuf ) );
    return retMqtt;
}

/*-----------------------------------------------------------*/

static BaseType_t resetCarError( obdContext_t * pObdContext )
{
    BaseType_t retMqtt = pdPASS;
    char messageId[ OBD_MESSAGE_ID_MAX ] = { 0 };
    uint32_t msgLength = 0;
    
    snprintf( messageId, OBD_MESSAGE_ID_MAX, "%s-%s", pObdContext->vin, pObdContext->isoTime );
    
    snprintf( pObdContext->topicBuf, OBD_TOPIC_BUF_SIZE, OBD_MAINTENANCE_TOPIC, pObdContext->thingName );
    
    /* Anomalies. */
    msgLength = snprintf( pObdContext->messageBuf, OBD_MESSAGE_BUF_SIZE, OBD_MAINTENANCE_FORMAT,
              messageId,                // MessageId
              pObdContext->isoTime,     // CreationTimeStamp
              pObdContext->isoTime,     // SendTimeStamp
              pObdContext->vin,         // vin
              // maintenance
              "anomalies",              // ID
              "A:OilTemp"               // Val
    );

    retMqtt = mqttAgentPublish( OBD_MQTT_QOS,
                                pObdContext->topicBuf,
                                strlen( pObdContext->topicBuf ),
                                pObdContext->messageBuf,
                                msgLength );

    if( retMqtt == pdPASS )
    {
        /* DTC. */
        msgLength = snprintf( pObdContext->messageBuf, OBD_MESSAGE_BUF_SIZE, OBD_MAINTENANCE_FORMAT,
                  messageId,                // MessageId
                  pObdContext->isoTime,     // CreationTimeStamp
                  pObdContext->isoTime,     // SendTimeStamp
                  pObdContext->vin,         // vin
                  // maintenance
                  "trouble_codes",          // ID
                  ""                        // Val
        );

        retMqtt = mqttAgentPublish( OBD_MQTT_QOS,
                                    pObdContext->topicBuf,
                                    strlen( pObdContext->topicBuf ),
                                    pObdContext->messageBuf,
                                    msgLength );
    }
    return retMqtt;
}

/*-----------------------------------------------------------*/

void vehicleTelemetryReportTask( void )
{
    uint64_t loopSteps = 0;
    uint32_t startTicksMs = 0, elapsedTicksMs = 0;
    bool ignitionStatus = false;
    int i = 0;
    double vehicleSpeed = 0;
    double gpsSpeed = 0;
    bool useSimulatledGPSData = false;

    gObdContext.buzzDevice = FreeRTOS_open( ( int8_t * )( "/dev/buzz" ), 0 );

    if( gObdContext.buzzDevice != NULL )
    {
        /* Network connection success indication. */
        buzz_beep( gObdContext.buzzDevice, BUZZ_SHORT_BEEP_DURATION_MS, 1 );
    }

    /* Open the OBD devices. */
    CMS_LOGI( TAG, "Start obd device init." );
    gObdContext.obdDevice = FreeRTOS_open( ( int8_t *) ( "/dev/obd" ), 0 );

    if( gObdContext.obdDevice == NULL )
    {
        CMS_LOGE( TAG, "OBD device open failed." );
    }
    else
    {
        for( i = 0; i < MAX_RETRY_TIMES; i++ )
        {
            if( OBDLib_Init( gObdContext.obdDevice ) != 0 )
            {
                vTaskDelay( pdMS_TO_TICKS(1000) );
            }
            else
            {
                gObdContext.obdDeviceConnected = true;
                break;
            }
        }
        if( gObdContext.obdDeviceConnected == true )
        {
            CMS_LOGI( TAG, "OBD device connected." );
            /* OBD connection indication. */
            buzz_beep( gObdContext.buzzDevice, BUZZ_SHORT_BEEP_DURATION_MS, 2 );
        }
        else
        {
            CMS_LOGW( TAG, "OBD device not connected. Use simulated vehicle speed." );
            /* OBD connection indication. */
            buzz_beep( gObdContext.buzzDevice, BUZZ_LONG_BEEP_DURATION_MS, 1 );
            vTaskDelay( pdMS_TO_TICKS( 100 ) );
            buzz_beep( gObdContext.buzzDevice, BUZZ_SHORT_BEEP_DURATION_MS, 1 );
        }
    }

    /* Read VIN and client ID. */
    #ifdef OBD_DEFAULT_VIN
        Peripheral_Descriptor_t secureDevice = FreeRTOS_open( ( int8_t * )( "/dev/secure" ), 0 );
        FreeRTOS_ioctl( secureDevice, ioctlSECURE_VIN, gObdContext.vin );
        FreeRTOS_ioctl( secureDevice, ioctlSECURE_CLIENT_ID, gObdContext.thingName );
    #else
        if( ODBLib_GetVIN( gObdContext.obdDevice, gObdContext.vin, OBD_VIN_MAX ) == false )
        {
            CMS_LOGE( TAG, "OBD device read VIN fail." );
        }
        else
        {
            CMS_LOGI( TAG, "OBD vin %s.", gObdContext.vin );
        }
    #endif /* ifdef OBD_DEFAULT_VIN */
    CMS_LOGD( TAG, "thing name is : %s.", gObdContext.thingName );

    /* Enable GPS device. */
    GPSLib_Begin( gObdContext.obdDevice );

    /* the external trip loop. */
    while( true )
    {
        resetTelemetryData( &gObdContext );

        updateTimestamp( &gObdContext, NULL );

        /* Reset car previous error. */
        if( pdFAIL == resetCarError( &gObdContext ) )
        {
            CMS_LOGE( TAG, "Failed to reset car Error" );
        }

        loopSteps = 0;
        ignitionStatus = false;
        useSimulatledGPSData = false;

        /* Main thread runs in send data loop. */
        while( true )
        {
            startTicksMs = xTaskGetTickCountMs();

            updateTimestamp( &gObdContext, NULL );

            /* Check the DTC events. */
            if( pdFAIL == checkObdDtcData( &gObdContext ) )
            {
                CMS_LOGE( TAG, "Failed to check obd DTC data" );
            }

            /* Check the Location data events. */
            gpsSpeed = updateGPSData( &gObdContext, useSimulatledGPSData );

            /* Check the ignition status. */
            if( ignitionStatus == false )
            {
                /* Wait until there is speed. */
                vehicleSpeed = obdReadVehicleSpeed( &gObdContext );

                if( ( vehicleSpeed <= CAR_IDLE_SPEED_THRESHOLD ) && ( gpsSpeed <= CAR_IDLE_SPEED_THRESHOLD ) )
                {
                    vTaskDelay( pdMS_TO_TICKS(OBD_DATA_COLLECT_INTERVAL_MS) );
                    continue;
                }
                else
                {
                    ignitionStatus = true;
                    if( gpsSpeed == 0 )
                    {
                        CMS_LOGW( TAG, "GPS is not ready, use simulated GPS data." );
                        useSimulatledGPSData = true;
                        gObdContext.startLatitude = 0;
                        gObdContext.startLongitude = 0;
                        gObdContext.startDirection = 0;
                    }

                    CMS_LOGD( TAG, "Vehicle speed %lf gps speed %lf for ignition.", vehicleSpeed, gpsSpeed );

                    /* Use time info as trip ID. */
                    genTripId( &gObdContext );
                    CMS_LOGI( TAG, "Start a new trip id %s.", gObdContext.tripId );

                    /* Save the start information. */
                    gObdContext.startTicksMs = ( uint64_t ) xTaskGetTickCountMs();
                    gObdContext.lastUpdateTicksMs = gObdContext.startTicksMs;
                    updateTelemetryData( &gObdContext );
                    strcpy( gObdContext.obdAggregatedData.start_time, gObdContext.isoTime );
                    gObdContext.start_fuel_level = gObdContext.fuel_level;

                    /* save the ignition event. */
                    strncpy( gObdContext.ignition_status, "run", OBD_IGNITION_MAX );
                }
            }
            else
            {
                /* Update telemetry data. */
                updateTelemetryData( &gObdContext );
            }

            /* Check the ignition off events. */
            if( ( gObdContext.obdDeviceConnected == false ) && ( gObdContext.updateCount > OBD_SIMULATED_TRIP_STEPS ) )
            {
                CMS_LOGD( TAG, "Simulated Trip Idle speed duration." );
                break;
            }
            else if( ( gObdContext.idleSpeedDurationIntervalMs >= ( ( uint64_t ) CAR_IGINITION_IDLE_OFF_MS ) ) &&
                ( gpsSpeed == 0 ) )
            {
                CMS_LOGI( TAG, "Idle speed duration interval ms %u %u.",
                        ( gObdContext.idleSpeedDurationIntervalMs <= UINT32_MAX ) ? ( uint32_t ) gObdContext.idleSpeedDurationIntervalMs : 0,
                        ( uint32_t ) CAR_IGINITION_IDLE_OFF_MS );
                /* save the ignition event. */
                strncpy( gObdContext.ignition_status, "off", OBD_IGNITION_MAX );
                break;
            }

            /* Check the telemetry data events. */
            if( ( loopSteps % OBD_TELEMETRY_DATA_INTERVAL_STEPS ) == 0 )
            {
                if( pdFAIL == sendObdTelemetryData( &gObdContext ) )
                {
                    CMS_LOGE( TAG, "Failed to send OBD telemetry data" );
                }
            }

            /* Calculate remain time. */
            elapsedTicksMs = xTaskGetTickCountMs() - startTicksMs;

            if( OBD_DATA_COLLECT_INTERVAL_MS > elapsedTicksMs )
            {
                if( ( OBD_DATA_COLLECT_INTERVAL_MS - elapsedTicksMs ) < 10 )
                {
                    vTaskDelay( pdMS_TO_TICKS(10) );
                }
                else
                {
                    vTaskDelay( pdMS_TO_TICKS(OBD_DATA_COLLECT_INTERVAL_MS - elapsedTicksMs) );
                }
            }
            else
            {
                CMS_LOGW( TAG, "elapsed time %u ms too long %u.",
                          elapsedTicksMs, OBD_DATA_COLLECT_INTERVAL_MS );
                vTaskDelay( pdMS_TO_TICKS(10) );
            }

            loopSteps = loopSteps + 1;
        }

        /* Send the trip data. */
        updateTelemetryData( &gObdContext );
        strcpy( gObdContext.transmission_gear_position, "neutral" );
        if( pdFAIL == sendObdTripData( &gObdContext ) )
        {
            CMS_LOGE( TAG, "Failed to send OBD trip data" );
        }
    }

    /* Delete the task if it is complete. */
    CMS_LOGI( TAG, "Task vehicleTelemetryReportTask completed." );
    vTaskDelete( NULL );
}

/*-----------------------------------------------------------*/
