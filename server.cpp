#include <agrpc/asio_grpc.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>
#include <exec/timed_thread_scheduler.hpp>

#include "pubsub.grpc.pb.h"

#include <chrono>
#include <iostream>
#include <thread>

using StreamingRPC = agrpc::ServerRPC<&PubSub::AsyncService::Requestsubscribe>;

exec::timed_thread_context timer;

struct ScopeExit {
  explicit ScopeExit(std::function<void()> func) noexcept
      : m_func(std::move(func)) {}
  ScopeExit(const ScopeExit &) = delete;
  ScopeExit(ScopeExit &&) = delete;
  ~ScopeExit() { m_func(); }

private:
  std::function<void()> m_func;
};

auto handle_streaming_request(StreamingRPC &rpc, google::protobuf::Empty &)
    -> exec::task<void> {
  std::cout << "dummy_request never finishes" << std::endl;
  ScopeExit exit_log(
      []() { std::cout << "dummy_request stopped" << std::endl; });
  co_await exec::schedule_after(timer.get_scheduler(), std::chrono::hours(1));
}

int main(int argc, const char **argv) {
  PubSub::AsyncService service;

  std::string url("0.0.0.0:50051");
  grpc::ServerBuilder builder;
  builder.AddListeningPort(url, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  auto grpc_context = agrpc::GrpcContext(builder.AddCompletionQueue());

  auto server = builder.BuildAndStart();

  grpc_context.work_started();
  const auto context_thread =
      std::jthread([&](std::stop_token) { grpc_context.run(); });
  exec::async_scope scope;

  scope.spawn(stdexec::schedule(grpc_context.get_scheduler()) |
              stdexec::let_value([&]() {
                return agrpc::register_sender_rpc_handler<StreamingRPC>(
                    grpc_context, service, &handle_streaming_request);
              }) |
              stdexec::upon_error([](auto) {
                std::cout << "register_sender_rpc_handler exception"
                          << std::endl;
              }) |
              stdexec::upon_stopped([]() {
                std::cout << "register_sender_rpc_handler stopped" << std::endl;
              }));

  std::cout << "press any key to stop the server" << std::endl;
  std::cin.get();
  scope.request_stop();
  server->Shutdown(std::chrono::system_clock::now());
  stdexec::sync_wait(scope.on_empty());
  grpc_context.work_finished();
  return 0;
}