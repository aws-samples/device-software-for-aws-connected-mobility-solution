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
 * @file FreeRTOS_IO.h
 * @brief Functions to interact with FreeRTOS driver interface.
 */

#ifndef FREERTOS_IO_H
#define FREERTOS_IO_H

#include "freertos/FreeRTOS.h"

#define FREERTOS_IO_OKAY               ( 0 )
#define FREERTOS_IO_ERROR_BAD_PARAM    ( -1 )

typedef const void * Peripheral_Descriptor_t;

/**
 * @brief Open a peripheral.
 *
 * @param[in] pcPath access path to the peripheral.
 * @param[in] ulFlags parameters to be used for opening peripheral.
 *
 * @return peripheral descriptor.
 */
Peripheral_Descriptor_t FreeRTOS_open( const int8_t * pcPath,
                                       const uint32_t ulFlags );

/**
 * @brief Read data from a peripheral.
 *
 * @param[in] pxPeripheral peripheral to be accessed.
 * @param[in] pvBuffer pointer to a buffer for receiving data.
 * @param[in] xBytes size of buffer for receiving data.
 *
 * @return length of data that read back.
 */
size_t FreeRTOS_read( Peripheral_Descriptor_t const pxPeripheral,
                      void * const pvBuffer,
                      const size_t xBytes );

/**
 * @brief Write data into a peripheral.
 *
 * @param[in] pxPeripheral peripheral to be accessed.
 * @param[in] pvBuffer pointer to a buffer for data to write.
 * @param[in] xBytes length of data to write.
 *
 * @return length of data that written successfully.
 */
size_t FreeRTOS_write( Peripheral_Descriptor_t const pxPeripheral,
                       const void * pvBuffer,
                       const size_t xBytes );

/**
 * @brief Send control command to a peripheral.
 *
 * @param[in] pxPeripheral peripheral to be accessed.
 * @param[in] ulRequest request of the control command.
 * @param[in] pvValue pointer to the data to send with.
 *
 * @return pdTRUE if command is sent successfully.
 * Otherwise return pdFALSE.
 */
BaseType_t FreeRTOS_ioctl( Peripheral_Descriptor_t const pxPeripheral,
                           uint32_t ulRequest,
                           void * pvValue );

#endif /*  FREERTOS_IO_H */
