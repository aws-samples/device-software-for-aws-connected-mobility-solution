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
 * @file obd_data.h
 * @brief Defines obd data structures.
 */

#ifndef  OBD_DATA_H
#define OBD_DATA_H

typedef enum ObdTelemetryDataType
{
    OBD_TELEMETRY_TYPE_STEERING_WHEEL_ANGLE = 0,
    OBD_TELEMETRY_TYPE_ACCELERATION,
    OBD_TELEMETRY_TYPE_IGNITION_STATUS,
    OBD_TELEMETRY_TYPE_OIL_TEMP,               /* */
    OBD_TELEMETRY_TYPE_ENGINE_SPEED,           /* */
    OBD_TELEMETRY_TYPE_VEHICLE_SPEED,          /* */
    OBD_TELEMETRY_TYPE_TORQUE_AT_TRANSMISSION, /* */
    OBD_TELEMETRY_TYPE_TRANSMISSION_GEAR_POSITION,
    OBD_TELEMETRY_TYPE_ACCELERATOR_PEDAL_POSITION,
    OBD_TELEMETRY_TYPE_FUEL_LEVEL,          /* */
    OBD_TELEMETRY_TYPE_FUEL_CONSUMED_SINCE_RESTART,
    OBD_TELEMETRY_TYPE_ODOMETER,            /* ioctlOBD_READ_PID_ODOMETER */
    OBD_TELEMETRY_TYPE_BRAKE_PEDAL_STATUS,
    OBD_TELEMETRY_TYPE_GEAR_LEVER_POSITION, /* ioctlOBD_READ_PID_TRANSMISSION_ACTUAL_GEAR */
    OBD_TELEMETRY_TYPE_BRAKE,
    OBD_TELEMETRY_TYPE_MAX
} ObdTelemetryDataType_t;

typedef struct ObdAggregatedData
{
    double vehicle_speed_mean;
    double vehicle_speed_max;
    double engine_speed_mean;
    double torque_at_transmission_mean;
    double oil_temp_mean;
    double accelerator_pedal_position_mean;
    double brake_mean;
    uint32_t high_acceleration_event;
    uint32_t high_braking_event;
    char start_time[ 64 ]; /* e.g., 1970-02-03 02 =59 =37.401000000 */
    double latitude;
    double longitude;
} ObdAggregatedData_t;

typedef struct ObdTelemetryData
{
    double steering_wheel_angle;
    double acceleration;
    double oil_temp;
    double engine_speed;
    double vehicle_speed;
    double torque_at_transmission;
    double accelerator_pedal_position;
    char gear_lever_position[ 16 ];
    double brake;
    bool parking_break_status;
} ObdTelemetryData_t;

#endif /* OBD_DATA_H */
