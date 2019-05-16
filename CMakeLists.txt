cmake_minimum_required (VERSION 3.1)
project (vutura_utm CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#set(CMAKE_PREFIX_PATH /usr/lib)

# Set output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Common library
add_subdirectory(nng_event_loop)
add_subdirectory(vutura_common)

add_subdirectory(src/mavlink_node)

if ((NOT DEFINED ${PLATFORM}) OR (NOT ${PLATFORM} STREQUAL "Disco"))
	message("Compiling AIRMAP NODE")
	add_subdirectory(src/airmap_node)
else()
	message("PLATFORM ${PLATFORM}")
endif()
#add_subdirectory(src/unifly_node)
add_subdirectory(src/sim_node)
#add_subdirectory(src/pprz_node)
#add_subdirectory(src/avoidance_node)
#add_subdirectory(src/experimental_node)
#add_subdirectory(src/dronecode_sdk_node)

# create log folder
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/avoidance_logs")
