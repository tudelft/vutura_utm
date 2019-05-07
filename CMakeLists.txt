cmake_minimum_required (VERSION 3.1)
project (vutura_utm CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

#set(CMAKE_PREFIX_PATH /usr/lib)

# Set output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Common library
add_subdirectory(vutura_common)


add_subdirectory(src/mavlink_node)
add_subdirectory(src/airmap_node)
add_subdirectory(src/unifly_node)
add_subdirectory(src/sim_node)
add_subdirectory(src/avoidance_node)
