cmake_minimum_required (VERSION 3.1)
project (mavlink_node CXX)

find_package(nng CONFIG REQUIRED)

file(GLOB SRCS
	"*.cpp"
)

# For protobuf includes:
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../vutura_common)

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/mavlink/common>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
)

target_link_libraries(${PROJECT_NAME} vutura_common)
