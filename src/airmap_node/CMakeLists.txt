cmake_minimum_required (VERSION 3.1)
project (airmap_node CXX)

find_package(Protobuf REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_library(Curl_LIBRARIES curl)
find_library(Cryptopp_LIBRARIES cryptopp)


file(GLOB SRCS
	"*.cpp"
	)

file(GLOB PROTO_FILES
	"proto/*.proto"
	)

# Generate protobuf files
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_executable(${PROJECT_NAME} ${SRCS} ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
#	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../../> # for vutura_common
        )

target_link_libraries(${PROJECT_NAME} vutura_common nng_event_loop ${Curl_LIBRARIES} ${Protobuf_LIBRARIES} ${Cryptopp_LIBRARIES} nlohmann_json ${CMAKE_INSTALL_PREFIX}/lib/libpaho-mqttpp3.so)
