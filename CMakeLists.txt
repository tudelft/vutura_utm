cmake_minimum_required(VERSION 3.1)
project(vutura_utm)

# Require nng for messaging between nodes
find_package(nng CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/include/proto)

file(GLOB proto_packages "${CMAKE_SOURCE_DIR}/proto/*.proto")
message("${proto_packages}")
execute_process(COMMAND protoc --proto_path=${CMAKE_SOURCE_DIR}/proto --c_out=${CMAKE_SOURCE_DIR}/include/proto ${proto_packages})

file(GLOB proto_c_files ${CMAKE_SOURCE_DIR}/include/proto/*.c)
message("${proto_c_files}")

include_directories(include/mavlink/common)
include_directories(include)
add_executable(mavlink_node src/mavlink_node/mavlink_node.c ${proto_c_files})
target_link_libraries(mavlink_node nng::nng protobuf-c)
