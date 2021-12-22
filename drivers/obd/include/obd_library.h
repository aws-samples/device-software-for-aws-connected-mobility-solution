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
 * @file obd_library.h
 * @brief Functions to interact with obd dongle.
 */

#ifndef OBD_LIBRARY_H
#define OBD_LIBRARY_H

/**
 * @brief Send command to obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 * @param[in] pCmd pointer to the command to send.
 * @param[in] pBuf buffer to receive command response data.
 * @param[in] bufSize size of buffer to receive command response data.
 * @param[in] readTimeout timeout of command sequence in milisecond.
 *
 * @return size of received response data.
 */
size_t OBDLib_SendCommand( Peripheral_Descriptor_t obdDevice,
                           const char * pCmd,
                           char * pBuf,
                           uint32_t bufSize,
                           uint32_t readTimeout );

/**
 * @brief Initialize obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 *
 * @return obd initialization code.
 */
int OBDLib_Init( Peripheral_Descriptor_t obdDevice );

/**
 * @brief Read PID from obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 * @param[in] pid pid value that need to read.
 * @param[in] pResult pointer to a buffer to receive read back data.
 *
 * @return true for successful read.
 * Otherwise return false.
 */
bool OBDLib_ReadPID( Peripheral_Descriptor_t obdDevice,
                     uint8_t pid,
                     int * pResult );

/**
 * @brief Read DTC(Diagnostic Trouble Code) from obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 * @param[in] codes[] the array to receive read back codes.
 * @param[in] maxCodes the max count of code can be read.
 *
 * @return true for successful read.
 * Otherwise return false.
 */
int OBDLib_ReadDTC( Peripheral_Descriptor_t obdDevice,
                    uint16_t codes[],
                    uint8_t maxCodes );

/**
 * @brief Clear DTC(Diagnostic Trouble Code) from obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 */
void OBDLib_ClearDTC( Peripheral_Descriptor_t obdDevice );

/**
 * @brief Read VIN(Vehicle Identification Number) from obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 * @param[in] buffer pointer to a buffer to receive read back VIN.
 * @param[in] bufsize size of buffer to receive data.
 *
 * @return true for successful read.
 * Otherwise return false.
 */
bool ODBLib_GetVIN( Peripheral_Descriptor_t obdDevice,
                    char * buffer,
                    uint8_t bufsize );

/**
 * @brief Read UTC time from obd device.
 *
 * @param[in] obdDevice obd device peripheral descriptor.
 * @param[in] pUTCStr pointer to a buffer to receive read back time.
 * @param[in] bufsize size of buffer to receive data.
 *
 * @return true for successful read.
 * Otherwise return false.
 */
bool OBDLib_ReadUTCTime( Peripheral_Descriptor_t obdDevice,
                         char *pUTCStr, uint32_t bufferSize );

#endif /* OBD_LIBRARY_H */
