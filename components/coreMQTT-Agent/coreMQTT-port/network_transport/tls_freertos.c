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
 * @file tls_freertos.c
 * @brief TLS transport interface functions.
 */

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"

#include "tls_freertos.h"
#include "tls_freertos_transport.h"

/*-----------------------------------------------------------*/

extern TransportNetworkInterface_t transportNetworkESP32Wifi;
extern TransportNetworkInterface_t transportNetworkCellular;

/*-----------------------------------------------------------*/

static TransportNetworkInterface_t * transportNetworkInterfaces[] =
{
#ifdef CONFIG_COREMQTT_TRANSPORT_WIFI_ENABLED
    &transportNetworkESP32Wifi,
#endif
#ifdef CONFIG_COREMQTT_TRANSPORT_CELLULAR_ENABLED
    &transportNetworkCellular
#endif
};

typedef struct NetworkContext
{
    TransportNetworkInterface_t *pTransportNetworkInterface;
    void * transportNetworkContext;
} NetworkContext_t;

/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_FreeRTOS_Connect( NetworkContext_t * pNetworkContext,
                                           const char * pHostName,
                                           uint16_t port,
                                           const NetworkCredentials_t * pNetworkCredentials,
                                           uint32_t receiveTimeoutMs,
                                           uint32_t sendTimeoutMs )
{
    TlsTransportStatus_t tlsTransportStatus = TLS_TRANSPORT_INVALID_PARAMETER;
    if( ( pNetworkContext != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface->transportConnect != NULL ) )
    {
        tlsTransportStatus = pNetworkContext->pTransportNetworkInterface->transportConnect( pNetworkContext->transportNetworkContext,
                                                                                           pHostName,
                                                                                           port,
                                                                                           pNetworkCredentials,
                                                                                           receiveTimeoutMs,
                                                                                           sendTimeoutMs );
    }
    else
    {
        LogError(( "TLS_FreeRTOS_Connect : Invalid parameters" ));
    }
    return tlsTransportStatus;
}

/*-----------------------------------------------------------*/

void TLS_FreeRTOS_Disconnect( NetworkContext_t * pNetworkContext )
{
    if( ( pNetworkContext != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface->transportDisconnect != NULL ) )
    {
        pNetworkContext->pTransportNetworkInterface->transportDisconnect( pNetworkContext->transportNetworkContext );
    }
    else
    {
        LogError(( "TLS_FreeRTOS_Connect : Invalid parameters" ));
    }
}

/*-----------------------------------------------------------*/

int32_t TLS_FreeRTOS_recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t bytesToRecv )
{
    int32_t retRecv = 0;
    if( ( pNetworkContext != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface->transportRecv != NULL ) )
    {
        retRecv = pNetworkContext->pTransportNetworkInterface->transportRecv( pNetworkContext->transportNetworkContext,
                                                                              pBuffer,
                                                                              bytesToRecv );
    }
    else
    {
        LogError(( "TLS_FreeRTOS_recv : Invalid parameters" ));
    }
    return retRecv;
}

/*-----------------------------------------------------------*/

int32_t TLS_FreeRTOS_send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t bytesToSend )
{
    int32_t retSend = 0;
    if( ( pNetworkContext != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface->transportSend != NULL ) )
    {
        retSend = pNetworkContext->pTransportNetworkInterface->transportSend( pNetworkContext->transportNetworkContext,
                                                                              pBuffer,
                                                                              bytesToSend );
    }
    else
    {
        LogError(( "TLS_FreeRTOS_send : Invalid parameters" ));
    }
    return retSend;
}

/*-----------------------------------------------------------*/

NetworkContext_t * TLS_FreeRTOS_AllocNetworkContext( TransportNetworkType_t networkType )
{
    uint32_t networkIndex = 0;
    uint32_t transportNetworkInterfaceNum = sizeof( transportNetworkInterfaces ) / sizeof( TransportNetworkInterface_t * );
    NetworkContext_t * pNetworkContext = NULL;

    pNetworkContext = malloc( sizeof( NetworkContext_t ) );
    if( pNetworkContext != NULL )
    {
        memset( pNetworkContext, 0, sizeof( NetworkContext_t ) );
        for( ; networkIndex < transportNetworkInterfaceNum; networkIndex++ )
        {
            if( transportNetworkInterfaces[ networkIndex ]->networkType == networkType )
            {
                pNetworkContext->pTransportNetworkInterface = transportNetworkInterfaces[ networkIndex ];
                pNetworkContext->transportNetworkContext = transportNetworkInterfaces[ networkIndex ]->transportAllocContext();
                break;
            }
        }
        if( pNetworkContext->transportNetworkContext == NULL )
        {
            /* Either allocate faile or interface is not enabled. */
            LogError(( "Transport network not found %d", networkType ));
            free( pNetworkContext );
            pNetworkContext = NULL;
        }
    }
    else
    {
        LogError(( "TLS_FreeRTOS_AllocNetworkContext : Allocate network context failed" ));
    }
    return pNetworkContext;
}

/*-----------------------------------------------------------*/

void TLS_FreeRTOS_FreeContext( NetworkContext_t * pNetworkContext )
{
    if( ( pNetworkContext != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface->transportFreeContext != NULL ) )
    {
        pNetworkContext->pTransportNetworkInterface->transportFreeContext( pNetworkContext->transportNetworkContext );
        pNetworkContext->transportNetworkContext = NULL;
        free( pNetworkContext );
    }
    else
    {
        LogError(( "TLS_FreeRTOS_FreeContext : Invalid parameters" ));
    }
}

/*-----------------------------------------------------------*/

TlsTransportStatus_t TLS_FreeRTOS_Ioctl( NetworkContext_t * pNetworkContext,
                                         uint32_t ulRequest,
                                         void * pvValue )
{
    TlsTransportStatus_t tlsTransportStatus = TLS_TRANSPORT_SUCCESS;

    if( ( pNetworkContext != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface != NULL ) &&
        ( pNetworkContext->pTransportNetworkInterface->transportIoctl != NULL ) )
    {
        tlsTransportStatus = pNetworkContext->pTransportNetworkInterface->transportIoctl(
            pNetworkContext->transportNetworkContext, ulRequest, pvValue );
    }
    return tlsTransportStatus;
}

/*-----------------------------------------------------------*/
