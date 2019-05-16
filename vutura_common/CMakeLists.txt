cmake_minimum_required (VERSION 3.1)
project (vutura_common CXX)

find_package(Protobuf REQUIRED)
find_package(nng CONFIG REQUIRED)

file(GLOB SRCS
	"src/communicator.cpp"
	"src/config.cpp"
	"src/helper.cpp"
	)

file(GLOB HDRS
	"include/vutura_common/*.hpp"
	)

file(GLOB PROTO_FILES
	"proto/*.proto"
	)

# Generate protobuf files
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(${PROJECT_NAME} ${SRCS} ${HDRS} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(${PROJECT_NAME} ${Protobuf_LIBRARIES} nng::nng)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
	)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:include/vutura_common>
	)

target_include_directories(${PROJECT_NAME} PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/vutura_common>
	$<INSTALL_INTERFACE:include/vutura_common>
	)


#install(FILES {PROTO_HDRS} DESTINATION include/vutura_common)
