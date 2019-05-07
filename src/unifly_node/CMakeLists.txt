cmake_minimum_required (VERSION 3.1)
project (unifly_node CXX)

find_package(nlohmann_json REQUIRED)
find_library(Curl_LIBRARIES curl)

file(GLOB SRCS
	"*.cpp"
	)

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../../> # for vutura_common
	)

target_link_libraries(${PROJECT_NAME} vutura_common ${Curl_LIBRARIES})
