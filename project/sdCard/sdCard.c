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
 * @file sdCard.c
 * @brief Implementation of esp32 SD card initialization.
 */

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"

#ifdef CONFIG_IDF_TARGET_ESP32
    #include "driver/sdmmc_host.h"
#endif

// include log print header
#include "cms_log.h"

/*-----------------------------------------------------------*/

#define MOUNT_POINT CONFIG_FS_MOUNT_POINT

// ESP32-S2 doesn't have an SD Host peripheral, always use SPI:
#ifdef CONFIG_IDF_TARGET_ESP32S2
    #ifndef USE_SPI_MODE
        #define USE_SPI_MODE
    #endif // USE_SPI_MODE
    // on ESP32-S2, DMA channel must be the same as host id
    #define SPI_DMA_CHAN    host.slot
#endif //CONFIG_IDF_TARGET_ESP32S2

// DMA channel to be used by the SPI peripheral
#ifndef SPI_DMA_CHAN
    #define SPI_DMA_CHAN        ( 1 )
#endif //SPI_DMA_CHAN

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef CONFIG_FREEMATICS_ONEPLUS_B
    #define USE_SPI_MODE
    // Pin mapping when using SPI mode.
    // With this mapping, SD card can be used both in SPI and 1-line SD mode.
    // Note that a pull-up on CS line is required in SD mode.
    #define PIN_NUM_MISO        ( 19 )
    #define PIN_NUM_MOSI        ( 23 )
    #define PIN_NUM_CLK         ( 18 )
    #define PIN_NUM_CS          ( 5 )

#else
    #ifdef USE_SPI_MODE
        // Pin mapping when using SPI mode.
        // With this mapping, SD card can be used both in SPI and 1-line SD mode.
        // Note that a pull-up on CS line is required in SD mode.
        #define PIN_NUM_MISO    ( 2 )
        #define PIN_NUM_MOSI    ( 15 )
        #define PIN_NUM_CLK     ( 14 )
        #define PIN_NUM_CS      ( 13 )
    #endif //USE_SPI_MODE
#endif //FREEMATICS_ONEPLUS_B

/*-----------------------------------------------------------*/

static const char *TAG = "sdCard";

/*-----------------------------------------------------------*/

void sd_card_init( void )
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    const char mount_point[] = MOUNT_POINT;
    CMS_LOGI( TAG, "Initializing SD card." );

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
#ifndef USE_SPI_MODE
    CMS_LOGI (TAG, "Using SDMMC peripheral." );
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode( 15, GPIO_PULLUP_ONLY );   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode( 2, GPIO_PULLUP_ONLY );    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode( 4, GPIO_PULLUP_ONLY );    // D1, needed in 4-line mode only
    gpio_set_pull_mode( 12, GPIO_PULLUP_ONLY );   // D2, needed in 4-line mode only
    gpio_set_pull_mode( 13, GPIO_PULLUP_ONLY );   // D3, needed in 4- and 1-line modes

    ret = esp_vfs_fat_sdmmc_mount( mount_point, &host, &slot_config, &mount_config, &card );
#else
    CMS_LOGI( TAG, "Using SPI peripheral." );

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize( host.slot, &bus_cfg, SPI_DMA_CHAN );
    if (ret != ESP_OK) {
        CMS_LOGE( TAG, "Failed to initialize bus." );
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount( mount_point, &host, &slot_config, &mount_config, &card );
#endif //USE_SPI_MODE

    if ( ret != ESP_OK )
    {
        if ( ret == ESP_FAIL )
        {
            CMS_LOGE( TAG, "Failed to mount filesystem. "
                      "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option." );
        } else 
        {
            CMS_LOGE( TAG, "Failed to initialize the card (%s). "
                      "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name( ret ) );
        }
        return;
    }
    // Card has been initialized, print its properties
    sdmmc_card_print_info ( stdout, card );
}

/*-----------------------------------------------------------*/
