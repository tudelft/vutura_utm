cmake_minimum_required (VERSION 3.1)
project (experimental_node CXX)

file(GLOB SRCS
	"*.cpp"
)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME})
