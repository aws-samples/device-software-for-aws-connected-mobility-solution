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
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include <string.h>

/* Transport include. */
#include "esp_transport.h"
#include "esp_transport_tcp.h"

/* TLS includes. */
#include "esp_transport_ssl.h"

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"

/* TLS transport header. */
#include "tls_freertos.h"
#include "tls_freertos_transport.h"

/*-----------------------------------------------------------*/

struct TransportNetworkContext
{
    esp_transport_handle_t transport;
    uint32_t receiveTimeoutMs;
    uint32_t sendTimeoutMs;
};

/*-----------------------------------------------------------*/

TlsTransportStatus_t Transport_Wifi_Connect( TransportNetworkContext_t * pNetworkContext,
                                             const char * pHostName,
                                             uint16_t port,
                                             const NetworkCredentials_t * pNetworkCredentials,
                                             uint32_t receiveTimeoutMs,
                                             uint32_t sendTimeoutMs )
{
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;

    if( ( pNetworkContext == NULL ) ||
        ( pHostName == NULL ) ||
        ( pNetworkCredentials == NULL ) )
    {
        LogError(( "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                  "pHostName=%p, pNetworkCredentials=%p.",
                  pNetworkContext,
                  pHostName,
                  pNetworkCredentials ));
        return TLS_TRANSPORT_INVALID_PARAMETER;
    }
    
    pNetworkContext->transport = esp_transport_ssl_init();
    pNetworkContext->receiveTimeoutMs = receiveTimeoutMs;
    pNetworkContext->sendTimeoutMs = sendTimeoutMs;
    if (pNetworkCredentials->pAlpnProtos) {
        esp_transport_ssl_set_alpn_protocol(pNetworkContext->transport, pNetworkCredentials->pAlpnProtos);
    }

    if (pNetworkCredentials->disableSni) {
        esp_transport_ssl_skip_common_name_check(pNetworkContext->transport);
    }

    if (pNetworkCredentials->pRootCa) {
        esp_transport_ssl_set_cert_data_der(pNetworkContext->transport, (const char *)pNetworkCredentials->pRootCa, pNetworkCredentials->rootCaSize);
    }

    if (pNetworkCredentials->pClientCert) {
        esp_transport_ssl_set_client_cert_data_der(pNetworkContext->transport, (const char *)pNetworkCredentials->pClientCert, pNetworkCredentials->clientCertSize);
    }

    if (pNetworkCredentials->pPrivateKey) {
        esp_transport_ssl_set_client_key_data_der(pNetworkContext->transport, (const char *)pNetworkCredentials->pPrivateKey, pNetworkCredentials->privateKeySize);
    }

    if (esp_transport_connect(pNetworkContext->transport, pHostName, port, receiveTimeoutMs * 10 ) < 0) {
        returnStatus = TLS_TRANSPORT_CONNECT_FAILURE;
    } else {
        returnStatus = TLS_TRANSPORT_SUCCESS;
    }

    /* Clean up on failure. */
    if( returnStatus != TLS_TRANSPORT_SUCCESS )
    {
        LogError(( "(Network connection %p) Connection to %s failed.",
                  pNetworkContext,
                  pHostName ));
        if( pNetworkContext != NULL )
        {
            esp_transport_close( pNetworkContext->transport );
            esp_transport_destroy( pNetworkContext->transport );
        }
    }
    else
    {
        LogInfo(( "(Network connection %p) Connection to %s established.",
                  pNetworkContext,
                  pHostName ));
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

void Transport_Wifi_Disconnect( TransportNetworkContext_t * pNetworkContext )
{
    if (( pNetworkContext == NULL ) ) {
        LogError(( "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p.", pNetworkContext ));
        return;
    }

    /* Attempting to terminate TLS connection. */
    esp_transport_close( pNetworkContext->transport );

    /* Free TLS contexts. */
    esp_transport_destroy( pNetworkContext->transport );
}
/*-----------------------------------------------------------*/

int32_t Transport_Wifi_recv( TransportNetworkContext_t * pNetworkContext,
                             void * pBuffer,
                             size_t bytesToRecv )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) || 
        ( pBuffer == NULL) || 
        ( bytesToRecv == 0) ) {
        LogError(( "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                  "pBuffer=%p, bytesToRecv=%d.", pNetworkContext, pBuffer, bytesToRecv ));
        return TLS_TRANSPORT_INVALID_PARAMETER;
    }

    tlsStatus = esp_transport_read(pNetworkContext->transport, pBuffer, bytesToRecv, pNetworkContext->receiveTimeoutMs);
    if (tlsStatus < 0) {
        LogError(( "Reading failed, errno= %d", errno ));
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

int32_t Transport_Wifi_send( TransportNetworkContext_t * pNetworkContext,
                             const void * pBuffer,
                             size_t bytesToSend )
{
    int32_t tlsStatus = 0;

    if (( pNetworkContext == NULL ) || 
        ( pBuffer == NULL) || 
        ( bytesToSend == 0) ) {
        LogError(( "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                  "pBuffer=%p, bytesToSend=%d.", pNetworkContext, pBuffer, bytesToSend ));
        return TLS_TRANSPORT_INVALID_PARAMETER;
    }

    tlsStatus = esp_transport_write(pNetworkContext->transport, pBuffer, bytesToSend, pNetworkContext->sendTimeoutMs);
    if (tlsStatus < 0) {
        LogError(( "Writing failed, errno= %d %d", errno, tlsStatus ));
        return ESP_FAIL;
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

TransportNetworkContext_t * Transport_Wifi_AllocContext( void )
{
    return malloc( sizeof( TransportNetworkContext_t ) );
}

/*-----------------------------------------------------------*/

void Transport_Wifi_FreeContext( TransportNetworkContext_t * pNetworkContext )
{
    if( pNetworkContext != NULL )
    {
        free( pNetworkContext );
    }
}

/*-----------------------------------------------------------*/

TlsTransportStatus_t Transport_Wifi_Ioctl( TransportNetworkContext_t * pNetworkContext,
                                           uint32_t ulRequest,
                                           void * pvValue )
{
    TlsTransportStatus_t tlsTransportStatus = TLS_TRANSPORT_SUCCESS;
    uint32_t readTimeoutMs = 0;

    if( pNetworkContext == NULL )
    {
        LogError(( "Transport_Cellular_Ioctl : Invalid network context" ));
    }
    else
    {
        switch( ulRequest )
        {
            case TRANSPORT_IOCTL_RECV_TIMEOUT:
                if( pvValue == NULL )
                {
                    LogError(( "TRANSPORT_IOCTL_RECV_TIMEOUT bad param pvValue" ));
                    tlsTransportStatus = TLS_TRANSPORT_INVALID_PARAMETER;
                }
                else
                {
                    readTimeoutMs = *( ( uint32_t * ) pvValue );
                    pNetworkContext->receiveTimeoutMs = readTimeoutMs;
                }
                break;

            default:
                LogError(( "Transport_Cellular_Ioctl unsupported Ioctl" ));
                tlsTransportStatus = TLS_TRANSPORT_INVALID_PARAMETER;
                break;
        }
    }

    return tlsTransportStatus;
}

/*-----------------------------------------------------------*/

TransportNetworkInterface_t transportNetworkESP32Wifi =
{
    .networkType = TRANSPORT_NETWORK_WIFI,
    .transportConnect = Transport_Wifi_Connect,
    .transportDisconnect = Transport_Wifi_Disconnect,
    .transportRecv = Transport_Wifi_recv,
    .transportSend = Transport_Wifi_send,
    .transportAllocContext = Transport_Wifi_AllocContext,
    .transportFreeContext = Transport_Wifi_FreeContext,
    .transportIoctl = Transport_Wifi_Ioctl
};

/*-----------------------------------------------------------*/
