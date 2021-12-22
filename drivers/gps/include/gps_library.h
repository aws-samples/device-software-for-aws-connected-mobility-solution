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
 * @file gps_library.h
 * @brief Functions to interact with OBD dongle gps.
 */

#ifndef GPS_LIBRARY_H
#define GPS_LIBRARY_H

/* This should be the same GPS_Data. */
typedef struct ObdGpsData
{
    uint32_t ts;
    uint32_t date;
    uint32_t time;
    double lat;
    double lng;
    double alt;        /* meter */
    double speed;      /* knot */
    uint16_t heading; /* degree */
    uint8_t hdop;
    uint8_t sat;
    uint16_t sentences;
    uint16_t errors;
} ObdGpsData_t;

/**
 * @brief Get GPS NMEA data.
 *
 * @param[in] obdDevice obd device descriptor.
 * @param[in] buffer buffer to receive read back data.
 * @param[in] bufsize size of the buffer to receive data.
 *
 * @return the size of data that read back.
 */
int GPSLib_GetNMEA( Peripheral_Descriptor_t obdDevice,
                    char * buffer,
                    int bufsize );

/**
 * @brief Start obd device GPS.
 *
 * @param[in] obdDevice obd device descriptor.
 *
 * @return true if GPS is started or false.
 */
bool GPSLib_Begin( Peripheral_Descriptor_t obdDevice );

/**
 * @brief Get GPS data.
 *
 * @param[in] obdDevice obd device descriptor.
 * @param[in] gpsData pointer to read back gps data.
 *
 * @return true if GPS date is read back or false.
 */
bool GPSLib_GetData( Peripheral_Descriptor_t obdDevice,
                     ObdGpsData_t * gpsData );

/**
 * @brief Stop obd device GPS.
 *
 * @param[in] obdDevice obd device descriptor.
 *
 * @return true if GPS is stopped or false.
 */
bool GPSLib_End( Peripheral_Descriptor_t obdDevice );

#endif /* GPS_LIBRARY_H */
