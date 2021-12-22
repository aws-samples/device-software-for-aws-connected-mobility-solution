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
 * @file main.c
 * @brief implementation of CMS application initialization and main loop.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "protocol_examples_common.h"

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IO.h"

#include "core_mqtt_agent.h"
#include "buzz_library.h"
#include "cms_log.h"

/*-----------------------------------------------------------*/

#define DOWNLOAD_AGENT_BUFFER_SIZE      ( 256 * 1024 )
#define JOB_START_DELAY_MS              ( 3000 )

#define DEMO_CONFIG_FILE_PATH           CONFIG_FS_MOUNT_POINT "/cms_demo_config.json"
#define MAX_RTSP_STREAMING_URL          ( 128U )

/* max retry count of syncing up with ntp server*/
#define OBD_OBTAIN_TIME_RETRY_COUNT     ( 20U )

/* ntp server sync interval*/
#define OBD_OBTAIN_TIME_LOOP_DELAY_MS   ( 2000U )

/* stack size of app obd telemetry task */
#define democonfigOBD_TELEMETRY_TASK_STACK_SIZE     ( 1024 * 8 )
#define democonfigDOWNLOAD_AGENT_TASK_STACK_SIZE    ( 1024 * 8 )

/*-----------------------------------------------------------*/

static const char *TAG = "main";

/*-----------------------------------------------------------*/

extern MQTTAgentContext_t xGlobalMqttAgentContext;

/*-----------------------------------------------------------*/

extern void sd_card_init(void);

static uint8_t * downloadAgentBuffer = NULL;
static char rtspStreamingUrl[ MAX_RTSP_STREAMING_URL ] = CONFIG_RTSP_STREAMING_URL;

extern void startMqttAgentTask( void );

extern void vehicleTelemetryReportTask(void);

/*-----------------------------------------------------------*/

static bool prvReadDemoConfig( void )
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
        CMS_LOGE( TAG, "Open %s failed.", DEMO_CONFIG_FILE_PATH );
        retKvsConfg = false;
    }
    else
    {
        fseek( fp, 0L, SEEK_END );
        sz = ftell( fp );
        fseek( fp, 0L, SEEK_SET );
        CMS_LOGI( TAG, "Open %s size %d.", DEMO_CONFIG_FILE_PATH, sz );
        pConfigBuffer = malloc( sz );
        if( pConfigBuffer == NULL )
        {
            CMS_LOGE( TAG, "Malloc buffer size %d failed.", sz );
            retKvsConfg = false;
        }
        else
        {
            readSz = fread( pConfigBuffer, 1, sz, fp );
            if( readSz != sz )
            {
                CMS_LOGE( TAG, "Read file failed." );
                retKvsConfg = false;
            }
        }
        fclose( fp );
    }

    /* Parse the RTSP server name. */
    if( retKvsConfg == true )
    {
        pJson = cJSON_Parse( pConfigBuffer );
        if( pJson == NULL )
        {
            CMS_LOGE( TAG, "cJSON_Parse failed." );
        }
        else
        {
            pSub = cJSON_GetObjectItem( pJson, "RTSP_STREAMING_URL" );
            if( pSub == NULL )
            {
                CMS_LOGE( TAG, "cJSON_GetObjectItem RTSP_STREAMING_URL failed use config." );
            }
            else
            {
                CMS_LOGI( TAG, "RTSP_STREAMING_URL : %s.", pSub->valuestring );
                strncpy( rtspStreamingUrl, pSub->valuestring, MAX_RTSP_STREAMING_URL );
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

/*-----------------------------------------------------------*/

static void syncUpDateTime(void)
{
    CMS_LOGI( TAG, "Initializing SNTP." );
    sntp_setoperatingmode( SNTP_OPMODE_POLL );
    sntp_setservername( 0, "pool.ntp.org" );
    sntp_set_time_sync_notification_cb( NULL );
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode( SNTP_SYNC_MODE_SMOOTH );
#endif
    sntp_init();

    // wait for time to be set
    int retry = 0;
    while ( sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= OBD_OBTAIN_TIME_RETRY_COUNT )
    {
        CMS_LOGI( TAG, "Waiting for system time to be set... (%d/%u).", retry, OBD_OBTAIN_TIME_RETRY_COUNT );
        vTaskDelay( pdMS_TO_TICKS( OBD_OBTAIN_TIME_LOOP_DELAY_MS ) );
    }

    if( retry < OBD_OBTAIN_TIME_RETRY_COUNT )
    {
        /* Set timezone */
        unsetenv( "TZ" );
        tzset();
    }
    else
    {
        CMS_LOGE( TAG, "Failed to sync up date and time." ); 
    }

}

/*-----------------------------------------------------------*/

void app_main()
{
    CMS_LOGI( TAG, "[APP] Startup.." );
    CMS_LOGI( TAG, "[APP] Free memory: %d bytes.", esp_get_free_heap_size() );
    CMS_LOGI( TAG, "[APP] IDF version: %s.", esp_get_idf_version() );

#ifdef CONFIG_FREEMATICS_ONEPLUS_B
    // Setup 5V output to driver ESP32
    gpio_config_t io_conf;
    // Disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // Set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // Bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ( 1ULL<< 12 );
    // Disable pull-down mode
    io_conf.pull_down_en = 0;
    // Disable pull-up mode
    io_conf.pull_up_en = 0;
    // Configure GPIO with the given settings
    gpio_config( &io_conf );
    
    gpio_set_direction( GPIO_NUM_12, GPIO_MODE_OUTPUT );
    gpio_set_level( GPIO_NUM_12, 1 );
#endif

    // Initialize network
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Establish network connection (either wifi or ethernet)
    ESP_ERROR_CHECK(example_connect());

    // Sync up data and time
    syncUpDateTime();

    // Read configurations from SD card
    #ifdef CONFIG_FILE_SYSTEM_ENABLE
        sd_card_init();
        prvReadDemoConfig();
    #endif

    // Start MQTT Agent
    startMqttAgentTask();

    // Wait until mqtt is connected
    while( MQTTNotConnected == xGlobalMqttAgentContext.mqttContext.connectStatus )
    {
        CMS_LOGI( TAG, "Waiting for MQTT to connect." );
        vTaskDelay( pdMS_TO_TICKS(1000) );
    }

    // Create vehicle telemetry report app
    xTaskCreate( (TaskFunction_t) vehicleTelemetryReportTask,
                 "vehicleTelemetryReportTask",
                 democonfigOBD_TELEMETRY_TASK_STACK_SIZE,
                 NULL,
                 tskIDLE_PRIORITY+1,
                 NULL
                );
}

/*-----------------------------------------------------------*/
