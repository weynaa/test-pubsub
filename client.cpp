#include <agrpc/asio_grpc.hpp>

#include "pubsub.grpc.pb.h"

int main(int argc, const char **argv) {
  agrpc::GrpcContext grpc_context;
  grpc_context.work_started();

  const auto grpc_context_thread = std::jthread([&]() { grpc_context.run(); });
  auto scope_exit = std::shared_ptr<agrpc::GrpcContext>(
      &grpc_context,
      [](agrpc::GrpcContext *context) { context->work_finished(); });
  using ClientRPC = agrpc::ClientRPC<&PubSub::AsyncService::Requestsubscribe>;


  return 0;
}