#
# Component Makefile
#

set( includes )
set( srcs )
set( priv_includes )

list( APPEND includes 
    "./include"    
)

list( APPEND srcs 
    "./source/obd_library.c"
)

list( APPEND priv_includes 
    "$ENV{IDF_PATH}/components/freertos/include/freertos"
)

idf_component_register( SRCS ${srcs} 
                        INCLUDE_DIRS ${includes}
                        PRIV_INCLUDE_DIRS ${priv_includes}
                        REQUIRES freertos freertos_plus_io
)