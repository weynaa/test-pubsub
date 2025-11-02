#include <agrpc/asio_grpc.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include "pubsub.grpc.pb.h"

#include <iostream>
#include <memory>
#include <thread>

struct AgrpcServer{

	using StreamingRPC = agrpc::ServerRPC<&PubSub::AsyncService::Requestsubscribe>;

	AgrpcServer(std::unique_ptr<grpc::Server> _server, std::unique_ptr<agrpc::GrpcContext> _grpc_context, PubSub::AsyncService & _service) : 
		server(std::move(_server)),
		service(_service), 
		grpc_context(std::move(_grpc_context)),
		context_thread(start_thread(*grpc_context)) {
			scope.spawn(stdexec::schedule(grpc_context->get_scheduler())
				| stdexec::let_value([this](){
					return agrpc::register_sender_rpc_handler<StreamingRPC>(*grpc_context, service, std::bind_front(&AgrpcServer::handle_streaming_request, this));
				}));
		}

	~AgrpcServer() {
		scope.request_stop();
		server->Shutdown();
		stdexec::sync_wait(scope.on_empty());
		grpc_context->work_finished();
		context_thread.join();
	}

private:
	auto handle_streaming_request(StreamingRPC &rpc) -> exec::task<void> {
		SubMessage msg;
		std::cout << "start streaming" << std::endl;
		while(co_await rpc.read(msg, agrpc::use_sender)){
			std::cout << "got message: " << msg.value() << std::endl;
		}
	}

	static auto start_thread(agrpc::GrpcContext& context) -> std::jthread { 
		context.work_started();
		return std::jthread([&context](std::stop_token){
			context.run();
		});
	}

	PubSub::AsyncService & service;
	std::unique_ptr<grpc::Server> server;
	std::unique_ptr<agrpc::GrpcContext> grpc_context;
	
	exec::async_scope scope;
	std::jthread context_thread;
};

int main(int argc, const char** argv){
	PubSub::AsyncService service;

	std::string url("0.0.0.0:50051");
	grpc::ServerBuilder builder;
	builder.AddListeningPort(url,grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	auto grpc_context = std::make_unique<agrpc::GrpcContext>(builder.AddCompletionQueue());
	
	auto server = builder.BuildAndStart();
	AgrpcServer agrpc_server(std::move(server), std::move(grpc_context), service);

	std::cout << "press any key to stop" << std::endl;
	std::cin.get();
	return 0;
}