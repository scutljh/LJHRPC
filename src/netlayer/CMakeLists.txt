cmake_minimum_required(VERSION 3.0)
project(NetLayer)

# 这一项一直不起效果
set(LIBRARY_OUTPUT_PATH,${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息，增加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 所有源文件放到src_list里
aux_source_directory(. SRC_LIST)
# 编译生成动态库
add_library(netlayer SHARED ${SRC_LIST})