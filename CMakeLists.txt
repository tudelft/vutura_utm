cmake_minimum_required(VERSION 3.1)
project(vutura_utm)

# Require nng for messaging between nodes
find_package(nng CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(cJSON REQUIRED)
find_package(nlohmann_json REQUIRED)


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

add_custom_command(
    PRE_BUILD
    COMMAND protoc --proto_path=${CMAKE_SOURCE_DIR}/proto --cpp_out=${CMAKE_SOURCE_DIR}/include/proto ${CMAKE_SOURCE_DIR}/proto/airmap_telemetry.proto ${CMAKE_SOURCE_DIR}/proto/messages.proto
    DEPENDS ${CMAKE_SOURCE_DIR}/proto/airmap_telemetry.proto
    OUTPUT ${CMAKE_SOURCE_DIR}/include/proto/airmap_telemetry.pb.cc ${CMAKE_SOURCE_DIR}/include/proto/airmap_telemetry.pb.h ${CMAKE_SOURCE_DIR}/include/proto/messages.pb.cc ${CMAKE_SOURCE_DIR}/include/proto/messages.pb.h
)

include_directories(include/mavlink/common)
include_directories(include)
add_executable(mavlink_node src/mavlink_node/mavlink_node.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h)
target_link_libraries(mavlink_node nng::nng protobuf-c)

add_executable(utm_node src/utm_node/utm_node.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h)
target_link_libraries(utm_node nng::nng protobuf-c)

include_directories(include/proto)
add_executable(airmap_node src/airmap_node/airmap_node.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.c ${CMAKE_SOURCE_DIR}/include/proto/messages.pb-c.h)
target_link_libraries(airmap_node nng::nng protobuf-c curl cjson nlohmann_json::nlohmann_json protobuf)

add_executable(airmap_telemetry src/airmap_node/airmap_telemetry.cpp  ${CMAKE_SOURCE_DIR}/include/proto/airmap_telemetry.pb.cc ${CMAKE_SOURCE_DIR}/include/proto/airmap_telemetry.pb.h ${CMAKE_SOURCE_DIR}/include/proto/messages.pb.cc ${CMAKE_SOURCE_DIR}/include/proto/messages.pb.h)
target_link_libraries(airmap_telemetry nlohmann_json::nlohmann_json protobuf cryptopp curl nng::nng)
