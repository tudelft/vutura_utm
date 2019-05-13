set(PLATFORM Disco)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
SET(CMAKE_C_COMPILER   /opt/arm-2012.03/bin/arm-none-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER /opt/arm-2012.03/bin/arm-none-linux-gnueabi-g++)

# Where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /opt/arm-2012.03/arm-none-linux-gnueabi/libc ${CMAKE_INSTALL_PREFIX})

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_MODULE_PATH "${CMAKE_FIND_ROOT_PATH}/")
