cmake_minimum_required (VERSION 3.1)
project (avoidance_node CXX)

find_package(nng CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_library(Polyclipping_LIBRARIES polyclipping)

file(GLOB SRCS
	"*.cpp"
)

#add_subdirectory(clipper)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} vutura_common nng_event_loop ${Polyclipping_LIBRARIES})
