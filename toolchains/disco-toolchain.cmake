set(PLATFORM Disco)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
SET(CMAKE_C_COMPILER   /opt/arm-2016.02-linaro/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER /opt/arm-2016.02-linaro/bin/arm-linux-gnueabihf-g++)

# Where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /opt/arm-2016.02-linaro/arm-linux-gnueabihf/libc ${CMAKE_INSTALL_PREFIX})

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_MODULE_PATH "${CMAKE_FIND_ROOT_PATH}/")
