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
 * @file obd_config.h
 * @brief Defines obd configurations.
 */

#ifndef OBD_CONFIG_H
#define OBD_CONFIG_H


/* The simulated car info. */
#define CAR_GAS_TANK_SIZE                      ( 50.0 )      /* Litres. */
#define CAR_HIGH_SPEED_THRESHOLD               ( 120.0 )     /* KM/hr. */
#define CAR_IDLE_SPEED_THRESHOLD               ( 1.0 )       /* KM/hr. */
#define CAR_IGINITION_IDLE_OFF_MS              ( 30 * 1000 ) /* Idle interval time to trigger iginition off. */
#define CAR_ACCELARATOR_PADEL_RPM_THRESHOLD    ( 6000.0 )

#define CAR_HIGH_OIL_TEMP_RPM                  ( 8000 )
#define CAR_HIGH_OIL_TEMP_RPM_DURATION_MS      ( 10000 )
#define CAR_HIGH_OIL_TEMP                      ( 300 )

/* The OBD data collect interval time. */
#define OBD_DATA_COLLECT_INTERVAL_MS           ( 2000 )

#define OBD_AGGREGATED_DATA_INTERVAL_MS        ( 20000 )
#define OBD_TELEMETRY_DATA_INTERVAL_MS         ( 2000 )

#define OBD_SIMULATED_TRIP_MS                  ( 120000 )
    /* Test code. <^ 25.03914, 121.563526 .*/
    /* Test code. >^ 25.03902, 121.568408 .*/
    /* Test code. >| 25.03290, 121.568386 .*/
    /* Test code. <| 25.03291, 121.563558 .*/
#define OBD_SIMULATED_TRIP_X1                   ( 25.03914 )
#define OBD_SIMULATED_TRIP_Y1                   ( 121.563526 )
#define OBD_SIMULATED_TRIP_X2                   ( 25.03290 )
#define OBD_SIMULATED_TRIP_Y2                   ( 121.568386 )

#define OBD_SIMULATED_VEHICLE_SPEED             ( 100.0 )

#define BUZZ_SHORT_BEEP_DURATION_MS             ( 250 )
#define BUZZ_LONG_BEEP_DURATION_MS              ( 1000 )

#endif /* OBD_CONFIG_H */
