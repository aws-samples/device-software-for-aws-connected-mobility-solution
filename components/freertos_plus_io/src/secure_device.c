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
 * @file secure_device.c
 * @brief Implementation of functions to access secure device io.
 */

#include <string.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IO.h"
#include "FreeRTOS_DriverInterface.h"

#include "secure_device.h"
#include "cJSON.h"

/*-----------------------------------------------------------*/

#define OBD_VIN_MAX_LENGTH      (32)
#define DEMO_CONFIG_FILE_PATH   CONFIG_FS_MOUNT_POINT "/cms_demo_config.json"

/*-----------------------------------------------------------*/

extern const uint8_t root_cert_auth_pem_start[]     asm("_binary_root_cert_auth_pem_start");
extern const uint8_t root_cert_auth_pem_end[]       asm("_binary_root_cert_auth_pem_end");

extern const uint8_t client_cert_pem_start[]    asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[]      asm("_binary_client_crt_end");

extern const uint8_t client_key_pem_start[]     asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[]       asm("_binary_client_key_end");

/*-----------------------------------------------------------*/

static const char client_id[] = CONFIG_MQTT_CLIENT_IDENTIFIER;
static const char broker_endpoint[] = CONFIG_MQTT_BROKER_ENDPOINT;
static char cms_vin[OBD_VIN_MAX_LENGTH] = CONFIG_CMS_VIN;

static Peripheral_Descriptor_t Secure_Open( const int8_t * pcPath,
                                            const uint32_t ulFlags );

static BaseType_t Secure_Ioctl( Peripheral_Descriptor_t const xPeripheral,
                                uint32_t ulRequest,
                                void * pvValue );

Peripheral_device_t gSecureDevice =
{
    "/dev/secure",
    Secure_Open,
    NULL, /* Empty Write function. */
    NULL, /* Empty Read function. */
    Secure_Ioctl,
    NULL
};

/*-----------------------------------------------------------*/

static bool prvReadVin( void )
{
    bool retKvsConfg = true;
    cJSON * pJson = NULL;
    cJSON * pSub = NULL;
    FILE * fp = NULL;
    size_t sz = 0, readSz = 0;
    char * pConfigBuffer = NULL;

    /* Check if file exist. */
    fp = fopen( DEMO_CONFIG_FILE_PATH, "rb" );
    if( fp == NULL )
    {
        printf( "Open %s failed\n", DEMO_CONFIG_FILE_PATH );
        retKvsConfg = false;
    }
    else
    {
        fseek( fp, 0L, SEEK_END );
        sz = ftell( fp );
        fseek( fp, 0L, SEEK_SET );
        printf( "Open %s size %d\n", DEMO_CONFIG_FILE_PATH, sz );
        pConfigBuffer = malloc( sz );
        if( pConfigBuffer == NULL )
        {
            printf( "Malloc buffer size %d failed\n", sz );
            retKvsConfg = false;
        }
        else
        {
            readSz = fread( pConfigBuffer, 1, sz, fp );
            if( readSz != sz )
            {
                printf( "Read file failed\n" );
                retKvsConfg = false;
            }
        }
        fclose( fp );
    }

    /* Parse VIN. */
    if( retKvsConfg == true )
    {
        pJson = cJSON_Parse( pConfigBuffer );
        if( pJson == NULL )
        {
            printf( "cJSON_Parse failed\n" );
        }
        else
        {
            pSub = cJSON_GetObjectItem( pJson, "VIN" );
            if( pSub == NULL )
            {
                printf( "cJSON_GetObjectItem VIN failed use config\n" );
            }
            else
            {
                printf( "VIN : %s\n", pSub->valuestring );
                strncpy( cms_vin, pSub->valuestring, OBD_VIN_MAX_LENGTH );
            }
        }
    }

    if( pJson != NULL )
    {
        cJSON_Delete( pJson );
    }
    if( pConfigBuffer != NULL )
    {
        free( pConfigBuffer );
    }

    return retKvsConfg;
}

Peripheral_Descriptor_t Secure_Open( const int8_t * pcPath,
                                   const uint32_t ulFlags )
{
    // Initialize VIN
    #ifdef CONFIG_FILE_SYSTEM_ENABLE
        prvReadVin();
    #endif

    Peripheral_Descriptor_t retDesc = NULL;
    retDesc = ( Peripheral_Descriptor_t ) &gSecureDevice;

    return retDesc;
}

/*-----------------------------------------------------------*/

BaseType_t Secure_Ioctl( Peripheral_Descriptor_t const pxPeripheral,
                       uint32_t ulRequest,
                       void * pvValue )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    BaseType_t retValue = pdPASS;

    uint32_t root_cert_auth_pem_length = ( uint32_t ) ( root_cert_auth_pem_end - root_cert_auth_pem_start );
    uint32_t client_cert_pem_length = ( uint32_t ) ( client_cert_pem_end - client_cert_pem_start );
    uint32_t client_key_pem_length = ( uint32_t ) ( client_key_pem_end -client_key_pem_start );

    /* configPRINTF(("%s:%d\r\n", __FUNCTION__, __LINE__)); */
    if( pDevice == NULL )
    {
        printf( "Secure_Ioctl bad param pxPeripheral\r\n" );
        retValue = pdFAIL;
    }
    else
    {
        switch( ulRequest )
        {
            case ioctlSECURE_ROOT_CA:
                if( pvValue != NULL )
                {
                    memcpy( pvValue, root_cert_auth_pem_start, root_cert_auth_pem_length );
                }
                break;
            case ioctlSECURE_CLIENT_CERT:
                if( pvValue != NULL )
                {
                    memcpy( pvValue, client_cert_pem_start, client_cert_pem_length );
                }
                break;
            case ioctlSECURE_CLIENT_KEY:
                if( pvValue != NULL )
                {
                    memcpy( pvValue, client_key_pem_start, client_key_pem_length );
                }
                break;
            case ioctlSECURE_CLIENT_ID:
                if( pvValue != NULL )
                {
                    memcpy( pvValue, client_id, strlen( client_id ) );
                }
                break;
            case ioctlSECURE_VIN:
                if( pvValue != NULL )
                {
                    memcpy( pvValue, cms_vin, strlen( cms_vin ) );
                }
                break;
            case ioctlSECURE_BROKER_ENDPOINT:
                if( pvValue != NULL )
                {
                    memcpy( pvValue, broker_endpoint, strlen( broker_endpoint ) );
                }
                break;
            case ioctlSECURE_BROKER_PORT:
                if( pvValue != NULL )
                {
                    *( ( uint32_t * ) pvValue ) = CONFIG_MQTT_BROKER_PORT;
                }
                break;
            default:
                break;
        }
    }

    return retValue;
}

/*-----------------------------------------------------------*/
