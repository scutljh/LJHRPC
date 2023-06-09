
# set(SRC_LIST userservice.cc ../user.pb.cc ../friend.pb.cc)
set(SRC_LIST friendservice.cc ../friend.pb.cc)
add_executable(provider ${SRC_LIST})

# 链接动态库
target_link_libraries(provider jhrpc protobuf) 