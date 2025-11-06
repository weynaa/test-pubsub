#include <agrpc/asio_grpc.hpp>
#include <grpcpp/grpcpp.h>

#include <stdexec/execution.hpp>

#include "pubsub.grpc.pb.h"

#include <iostream>

int main(int argc, const char **argv) {
  agrpc::GrpcContext grpc_context;
  grpc_context.work_started();

  const auto grpc_context_thread = std::jthread([&]() { grpc_context.run(); });
  auto scope_exit = std::shared_ptr<agrpc::GrpcContext>(
      &grpc_context,
      [](agrpc::GrpcContext *context) { context->work_finished(); });
  
  std::string url("127.0.0.1:50051");
  const auto channel = grpc::CreateChannel(url, grpc::InsecureChannelCredentials());
  PubSub::Stub stub(channel);

  using SubRPC = agrpc::ClientRPC<&PubSub::Stub::PrepareAsyncsubscribe>;
  SubRPC rpc(grpc_context);
  SubMessage msg;

  google::protobuf::Empty empty;
  stdexec::sync_wait(rpc.start(stub, empty));

  while(true){
    const auto opt_bool = stdexec::sync_wait(rpc.read(msg));
    if(!opt_bool.has_value() && !std::get<0>(*opt_bool)){
      break;
    }
    std::cout << "got message "<< msg.value() << std::endl;
  }

  return 0;
}