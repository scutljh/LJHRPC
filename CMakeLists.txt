cmake_minimum_required(VERSION 3.0)
project(jhrpc)

# 生成debug版本，可进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")


# 可执行文件输出
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 库文件输出
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# 编译头文件搜索路径 -I
include_directories((${PROJECT_SOURCE_DIR}/src/include))
include_directories((${PROJECT_SOURCE_DIR}/example))

# 库文件搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

# 去src中也执行其cmakelists
add_subdirectory(src)
add_subdirectory(example)