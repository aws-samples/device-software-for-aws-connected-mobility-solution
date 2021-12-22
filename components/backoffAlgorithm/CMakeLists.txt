#
# Component Makefile
#

set( includes )
set( srcs )
set( priv_includes )

set( BACKOFF_ALGORITHM_ROOT "${CMAKE_CURRENT_LIST_DIR}/backoffAlgorithm" )

list( APPEND includes
    ${BACKOFF_ALGORITHM_ROOT}/source/include
)

list( APPEND srcs
    ${BACKOFF_ALGORITHM_ROOT}/source/backoff_algorithm.c
)

list( APPEND priv_includes
    "$ENV{IDF_PATH}/components/freertos/include/freertos"
)

idf_component_register( SRCS ${srcs}
                        INCLUDE_DIRS ${includes}
                        PRIV_INCLUDE_DIRS ${priv_includes} )
