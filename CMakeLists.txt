cmake_minimum_required(VERSION 3.27)
project(test_pubsub)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(gRPC REQUIRED)

set(STDEXEC_BUILD_EXAMPLES OFF)
set(STDEXEC_BUILD_TESTS OFF)
add_subdirectory(deps/asio-grpc)
add_subdirectory(deps/stdexec)

add_library(proto STATIC)
target_link_libraries(proto PUBLIC gRPC::grpc++)
asio_grpc_protobuf_generate(
	PROTOS pubsub.proto
	OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto
	TARGET proto
	GENERATE_GRPC
)

target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)

add_executable(client client.cpp)
target_link_libraries(client PRIVATE proto asio-grpc::asio-grpc-stdexec STDEXEC::stdexec)

add_executable(server server.cpp)
target_link_libraries(server PRIVATE proto asio-grpc::asio-grpc-stdexec STDEXEC::stdexec)

add_executable(shutdown_bug_minimal shutdown_bug_minimal.cpp)
target_link_libraries(shutdown_bug_minimal PRIVATE proto asio-grpc::asio-grpc-stdexec STDEXEC::stdexec)

