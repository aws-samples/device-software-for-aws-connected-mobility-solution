# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set( COMPONENTS_PATH "../components" )
set( DRIVERS_PATH "../drivers" )

set( EXTRA_COMPONENT_DIRS 
        $ENV{IDF_PATH}/examples/common_components/protocol_examples_common
        ${COMPONENTS_PATH}/backoffAlgorithm
        ${COMPONENTS_PATH}/coreMQTT-Agent
        ${COMPONENTS_PATH}/freertos_plus_io
        ${DRIVERS_PATH}/buzzer
        ${DRIVERS_PATH}/gps
        ${DRIVERS_PATH}/obd
)

include( $ENV{IDF_PATH}/tools/cmake/project.cmake )
project( cms_demo )

# Credential files
if( NOT CREDENTIAL_FOLDER )
    set( CREDENTIAL_FOLDER credentials )
    message( WARNING "CREDENTIAL_FOLDER is not set, use ${CREDENTIAL_FOLDER}" )
endif()
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "${CREDENTIAL_FOLDER}/root_cert_auth.pem" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "${CREDENTIAL_FOLDER}/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "${CREDENTIAL_FOLDER}/client.key" TEXT)
