cmake_minimum_required(VERSION 3.1)
project(vutura_utm)

# Require nng for messaging between nodes
find_package(nng CONFIG REQUIRED)

include_directories(include/mavlink/common)
add_executable(mavlink_node src/mavlink_node/mavlink_node.c)
target_link_libraries(mavlink_node nng::nng)
