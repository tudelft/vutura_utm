cmake_minimum_required (VERSION 3.1)
project (dronecode_sdk_node CXX)

file(GLOB SRCS
	"*.cpp"
)

file(GLOB PROTO_FILES
	"proto/*.proto"
)

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} vutura_common dronecode_sdk dronecode_sdk_mission)


