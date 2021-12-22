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
 * @file tls_freertos_transport.h
 * @brief Defines data structures of transport interface.
 */

#ifndef TLS_FREERTOS_TRANSPORT_H
#define TLS_FREERTOS_TRANSPORT_H

#include "tls_freertos.h"

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Logging related header files are required to be included in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define LIBRARY_LOG_NAME and  LIBRARY_LOG_LEVEL.
 * 3. Include the header file "logging_stack.h".
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "TlsTransport"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#define SdkLog( message )    printf message

#include "logging_stack.h"

/************ End of logging configuration ****************/

typedef struct TransportNetworkContext TransportNetworkContext_t;

typedef struct TransportNetworkInterface
{
    TransportNetworkType_t networkType;
    TlsTransportStatus_t ( * transportConnect )( TransportNetworkContext_t * pTransportNetworkContext,
                                                 const char * pHostName,
                                                 uint16_t port,
                                                 const NetworkCredentials_t * pNetworkCredentials,
                                                 uint32_t receiveTimeoutMs,
                                                 uint32_t sendTimeoutMs );
    void ( * transportDisconnect )( TransportNetworkContext_t * pTransportNetworkContext );
    int32_t ( * transportRecv )( TransportNetworkContext_t * pTransportNetworkContext,
                           const void * pBuffer,
                           size_t bytesToRecv );
    int32_t ( * transportSend )( TransportNetworkContext_t * pTransportNetworkContext,
                           const void * pBuffer,
                           size_t bytesToSend );
    TransportNetworkContext_t * ( * transportAllocContext )( void );
    void ( * transportFreeContext )( TransportNetworkContext_t * pTransportNetworkContext );
    TlsTransportStatus_t ( * transportIoctl )( TransportNetworkContext_t * pTransportNetworkContext,
                                               uint32_t ulRequest,
                                               void * pvValue );
} TransportNetworkInterface_t;


#endif /* TLS_FREERTOS_TRANSPORT_H */
