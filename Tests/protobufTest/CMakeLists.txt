message("hello")
message(${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(OUTPUT message.pb.cc message.pb.h
        COMMAND protoc
        ARGS --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/message.proto -I${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS message.proto)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)
link_libraries(protobuf)

add_executable(protobufTest protobufTest.cpp message.pb.cc)
