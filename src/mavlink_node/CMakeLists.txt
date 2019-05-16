cmake_minimum_required (VERSION 3.1)
project (mavlink_node CXX)

find_package(nng CONFIG REQUIRED)

file(GLOB SRCS
	"*.cpp"
)

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mavlink/common>
)

target_link_libraries(${PROJECT_NAME} nng_event_loop vutura_common)
