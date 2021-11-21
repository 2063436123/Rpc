cmake_minimum_required(VERSION 3.16)
project(testLinux)

set(CMAKE_CXX_STANDARD 20)

link_libraries(pthread gtest zookeeper_mt)

include_directories(third_party/Dagger/src)
include_directories(third_party/Dagger/third_party/fmt_include)

set(THIRD_SRCS
        third_party/Dagger/third_party/fmt_src/os.cc
        third_party/Dagger/third_party/fmt_src/format.cc
        third_party/Dagger/src/Sources/Socket.cpp
        third_party/Dagger/src/Sources/Epoller.cpp
        third_party/Dagger/src/Sources/TcpConnection.cpp
        third_party/Dagger/src/Sources/EventLoop.cpp
        third_party/Dagger/src/Sources/Logger.cpp
        third_party/Dagger/src/Sources/TcpServer.cpp
        third_party/Dagger/src/Sources/ObjectPool.cpp
        third_party/Dagger/src/Sources/Timer.cpp)
set(SRCS
        ${THIRD_SRCS}
        src/impl/ServiceRegistrationDiscovery.cpp)

# zookeeper test
add_executable(ServiceRegisterTest Tests/ServiceRegisterTest.cpp ${SRCS})
add_executable(ServiceDiscoverTest Tests/ServiceDiscoverTest.cpp ${SRCS})

# basic test
add_executable(RpcServer Tests/RpcServerTest.cpp ${SRCS})
add_executable(RpcClient Tests/RpcClientTest.cpp ${THIRD_SRCS})

# protobuf test
add_subdirectory(Tests/protobufTest)

# codec test
add_executable(CodecTest Tests/CodecTest.cpp ${SRCS})

# funcVerify test
add_executable(FuncVerifyTest Tests/FuncVerifyTest.cpp ${SRCS})

# logger stop test
add_executable(LoggerTest Tests/LoggerTest.cpp ${THIRD_SRCS})