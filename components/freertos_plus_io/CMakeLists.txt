#
# Component Makefile
#

set( includes )
set( srcs )
set( priv_includes )

set( FREERTOS_PLUS_IO_SOURCE "${CMAKE_CURRENT_LIST_DIR}/src" )

list(APPEND srcs
    "${FREERTOS_PLUS_IO_SOURCE}/FreeRTOS_DriverInterface.c"
    "${FREERTOS_PLUS_IO_SOURCE}/buzz_device.c"
    "${FREERTOS_PLUS_IO_SOURCE}/obd_device.c"
    "${FREERTOS_PLUS_IO_SOURCE}/secure_device.c"
)

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

list(APPEND priv_includes
    # This is add only because The freertos component path is not the same official
    "$ENV{IDF_PATH}/components/freertos/include/freertos"
)

idf_component_register( SRCS "${srcs}"
                        INCLUDE_DIRS ${includes}
                        PRIV_INCLUDE_DIRS ${priv_includes}
                        REQUIRES coreMQTT-Agent freertos esp32 tcp_transport json)
