# 将当前目录的所有源文件放到SRC_LIST中
add_subdirectory(netlayer)
aux_source_directory(. SRC_LIST)
add_library(jhrpc ${SRC_LIST})
target_link_libraries(jhrpc netlayer pthread zookeeper_mt)