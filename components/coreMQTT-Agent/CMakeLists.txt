#
# Component Makefile
#

set( includes )
set( srcs )
set( priv_includes )

set( COREMQTT_LIB           "${CMAKE_CURRENT_LIST_DIR}/coreMQTT-Agent/source/dependency/coreMQTT")
set( COREMQTT_AGENT_LIB     "${CMAKE_CURRENT_LIST_DIR}/coreMQTT-Agent" )
set( COREMQTT_PORT          "${CMAKE_CURRENT_LIST_DIR}/coreMQTT-port")
set( COREMQTT_AGENT_PORT    "${CMAKE_CURRENT_LIST_DIR}/coreMQTT-Agent-port" )

list( APPEND includes
    # coreMQTT Agent headers
    ${COREMQTT_AGENT_LIB}/source/include
    # coreMQTT Agent port headers
    ${COREMQTT_AGENT_PORT}/include
    #coreMQTT headers
    ${COREMQTT_LIB}/source/include
    ${COREMQTT_LIB}/source/interface
    # coreMQTT port headers
    ${COREMQTT_PORT}/include
    ${COREMQTT_PORT}/logging
    ${COREMQTT_PORT}/network_transport
    ${COREMQTT_PORT}/platform
    )

list( APPEND srcs
    # coreMQTT Agent source
    ${COREMQTT_AGENT_LIB}/source/core_mqtt_agent.c
    ${COREMQTT_AGENT_LIB}/source/core_mqtt_agent_command_functions.c
    # coreMQTT port source
    ${COREMQTT_AGENT_PORT}/source/freertos_agent_message.c
    ${COREMQTT_AGENT_PORT}/source/freertos_command_pool.c
    ${COREMQTT_AGENT_PORT}/source/subscription_manager.c
    ${COREMQTT_AGENT_PORT}/source/core_mqtt_agent_daemon.c
    ${COREMQTT_AGENT_PORT}/source/core_mqtt_agent_tasks.c
    # coreMQTT source
    ${COREMQTT_LIB}/source/core_mqtt_serializer.c
    ${COREMQTT_LIB}/source/core_mqtt_state.c
    ${COREMQTT_LIB}/source/core_mqtt.c
    # coreMQTT port source
    ${COREMQTT_PORT}/network_transport/tls_freertos.c
    ${COREMQTT_PORT}/network_transport/transport_esp32_wifi/transport_esp32_wifi.c
    ${COREMQTT_PORT}/platform/platform.c
    )

list( APPEND priv_includes
    "$ENV{IDF_PATH}/components/freertos/include/freertos"
)

idf_component_register( SRCS ${srcs}
                        INCLUDE_DIRS ${includes}
                        PRIV_INCLUDE_DIRS ${priv_includes}
                        REQUIRES backoffAlgorithm lwip nghttp mbedtls tcp_transport freertos )
