cmake_minimum_required(VERSION 3.1)
project(vutura_utm)

# Require nng for messaging between nodes
find_package(nng CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(cJSON REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/include/proto)

file(GLOB proto_packages "${CMAKE_SOURCE_DIR}/proto/*.proto")
#message("${proto_packages}")b
message("protoc --dependencies_out --proto_path=${CMAKE_SOURCE_DIR}/proto --c_out=${CMAKE_SOURCE_DIR}/include/proto ${proto_packages}")
add_custom_command(
    PRE_BUILD
    COMMAND protoc --proto_path=${CMAKE_SOURCE_DIR}/proto --c_out=${CMAKE_SOURCE_DIR}/include/proto ${proto_packages}
    DEPENDS ${CMAKE_SOURCE_DIR}/proto/messages.proto
    OUTPUT ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h
)

file(GLOB proto_c_files ${CMAKE_SOURCE_DIR}/include/proto/*.c)
#message("${proto_c_files}")

include_directories(include/mavlink/common)
include_directories(include)
add_executable(mavlink_node src/mavlink_node/mavlink_node.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h)
target_link_libraries(mavlink_node nng::nng protobuf-c)

add_executable(utm_node src/utm_node/utm_node.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h)
target_link_libraries(utm_node nng::nng protobuf-c)

add_executable(airmap_node src/airmap_node/airmap_node.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h)
target_link_libraries(airmap_node nng::nng protobuf-c curl cjson)
