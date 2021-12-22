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
 * @file obd_context.h
 * @brief Defines obd context structure.
 */

#ifndef OBD_CONTEXT_H
#define OBD_CONTEXT_H

#define OBD_ISO_TIME_MAX                       ( 64 )
#define OBD_VIN_MAX                            ( 32 )
#define OBD_IGNITION_MAX                       ( 8 )
#define OBD_TRANSMISSION_GEAR_POSITION_MAX     ( 10 )
#define OBD_TRIP_ID_MAX                        ( 16 )
#define OBD_TRIP_NAME_MAX                      ( 32 )
#define OBD_THINGNAME_MAX                      ( 32 )
#define OBD_MESSAGE_ID_MAX                     ( 128 )

#define OBD_MESSAGE_BUF_SIZE                   ( 2048 )
#define OBD_TOPIC_BUF_SIZE                     ( 64 )

typedef struct obdContext
{
    ObdAggregatedData_t obdAggregatedData;
    ObdTelemetryData_t obdTelemetryData;
    char thingName[ OBD_THINGNAME_MAX ];
    char tripId[ OBD_TRIP_ID_MAX ];
    char tripName[ OBD_TRIP_NAME_MAX ];
    char vin[ OBD_VIN_MAX ];
    char ignition_status[ OBD_IGNITION_MAX ];
    char transmission_gear_position[ OBD_TRANSMISSION_GEAR_POSITION_MAX ];
    double latitude;
    double longitude;
    double startLatitude;
    double startLongitude;
    uint8_t startDirection;
    double odometer;
    bool brake_pedal_status;
    double fuel_level; /* 0 to 100 in %. */
    double start_fuel_level;
    double fuel_consumed_since_restart;
    uint64_t highSpeedDurationMs;
    uint64_t idleSpeedDurationMs;
    uint64_t idleSpeedDurationIntervalMs;
    
    uint64_t higRpmDurationIntervalMs;
    
    ObdTelemetryDataType_t telemetryIndex;
    char topicBuf[ OBD_TOPIC_BUF_SIZE ];
    char messageBuf[ OBD_MESSAGE_BUF_SIZE ];
    Peripheral_Descriptor_t obdDevice;
    Peripheral_Descriptor_t buzzDevice;
    char isoTime[ OBD_ISO_TIME_MAX ];
    uint8_t timeSelection;
    uint64_t startTicksMs;
    uint64_t lastUpdateTicksMs; /* Uptime ticks. */
    uint32_t updateCount;
    bool gpsReceived;
    bool obdDeviceConnected;
} obdContext_t;

#endif /* OBD_CONTEXT_H */
