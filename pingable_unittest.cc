#include <iostream>
#include <string>

#include <thread>
#include <chrono>

#include "pingable.h"

#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel_server.h"

#include "base/threading/thread.h"
#include "base/test/task_environment.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/message_loop/message_pump.h"
#include "base/message_loop/message_pump_type.h"
#include "base/task/sequence_manager/sequence_manager_impl.h"

// Windows Specific headers
#include <windows.h>

using QueueName = ::perfetto::protos::pbzero::SequenceManagerTask::QueueName;

#define MOJO_BASIC_INITIALIZE  mojo::core::Init()
#define MOJO_INITIALIZE		\
	MOJO_BASIC_INITIALIZE; \
	base::Thread __ipc_thread("ipc!"); \
	__ipc_thread.StartWithOptions( \
		base::Thread::Options(base::MessagePumpType::IO, 0)); \
	mojo::core::ScopedIPCSupport ipc_support( \
		__ipc_thread.task_runner(), \
		mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

#define THREAD_INITIALIZE {\
	base::ThreadPoolInstance::Create("Default"); \
	auto param = base::ThreadPoolInstance::InitParams(10); \
	base::ThreadPoolInstance::Get()->Start(param); \
}

#define MAINTHREAD_SETUP \
	base::sequence_manager::SequenceManager::PrioritySettings __priSettings(10, 0); \
	auto __settings = base::sequence_manager::SequenceManager::Settings::Builder() \
						  .SetMessagePumpType(base::MessagePumpType::DEFAULT) \
						  .SetRandomisedSamplingEnabled(true) \
						  .SetAddQueueTimeToTasks(true) \
						  .SetPrioritySettings(std::move(__priSettings)) \
						  .Build(); \
	auto __seqM = CreateSequenceManagerOnCurrentThreadWithPump( \
		base::MessagePump::Create(base::MessagePumpType::DEFAULT), std::move(__settings)); \
	auto __q = __seqM->CreateTaskQueue(base::sequence_manager::TaskQueue::Spec{QueueName::DEFAULT_TQ}); \
	__seqM->SetDefaultTaskRunner(__q->task_runner());

void SingleProcess() {
	MOJO_INITIALIZE;
	MAINTHREAD_SETUP;

	mojo::Remote<example::mojom::Pingable> pingable;
	mojo::PendingReceiver<example::mojom::Pingable> receiver =
		pingable.BindNewPipeAndPassReceiver();

	std::unique_ptr<EXAMPLE_LOCAL::PingableImpl> pingable_global = 
		std::make_unique<EXAMPLE_LOCAL::PingableImpl>(std::move(receiver));

	pingable->Ping(base::BindOnce([](int a) {
		std::cout << "Ping: " << a << std::endl;
	}));

	base::RunLoop loop;
	loop.RunUntilIdle();
}

void receiver_side(mojo::NamedPlatformChannel& channel) {
	mojo::PlatformChannelServer server;
	server.Listen(
		channel.TakeServerEndpoint(),
		base::BindRepeating([](mojo::PlatformChannelEndpoint endpoint) {
			std::cout << "Connection Arrived" << std::endl;
		}));

	base::RunLoop loop;
	loop.Run();
}

void remote_side(std::string channel_name) {
	mojo::PlatformChannelEndpoint endpoint;
	std::wstring channel_name_w{ channel_name.begin(), channel_name.end() };

	endpoint = mojo::NamedPlatformChannel::ConnectToServer(channel_name_w);
	
	/*
	MOJO_BASIC_INITIALIZE;
	MAINTHREAD_SETUP;
	
	mojo::Remote<example::mojom::Pingable> pingable;
	mojo::PendingReceiver<example::mojom::Pingable> receiver =
		pingable.BindNewPipeAndPassReceiver();
	
	std::unique_ptr<EXAMPLE_LOCAL::PingableImpl> pingable_global = 
		std::make_unique<EXAMPLE_LOCAL::PingableImpl>(std::move(receiver));
	pingable->Ping(base::BindOnce([](int a) { 
		std::cout << "Ping: " << a << std::endl;
	}));
	*/
}



void CrossProcess(int argc, char* argv[]) {
	MOJO_INITIALIZE;
	MAINTHREAD_SETUP;

	if (argc == 1) {
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));

		// First, a MessagePipe bridge Receiver-Side and Remote-Side is
		// required. 
		mojo::NamedPlatformChannel::Options options;
		auto channel = mojo::NamedPlatformChannel(options);

		std::string cmd{ "example_unittest.exe remote" };
		std::wstring cmd_channel_w = channel.GetServerName();
		std::string cmd_channel{ cmd_channel_w.begin(), cmd_channel_w.end() };
		cmd = cmd + " " + cmd_channel;
		if (!CreateProcessA(
			argv[0], const_cast<char*>(cmd.c_str()), NULL,
			NULL, FALSE, 0,
			NULL, NULL, &si, &pi)) {
		
			std::cout << "Failed to create process" << std::endl;
		} else {
			receiver_side(channel);

			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
		}
	} else if (std::string{argv[1]} == "remote" && argc == 3) {
		remote_side(std::string{argv[2]});
	} else {
		std::cout << "No Match" << std::endl;
	}
}

int main(int argc, char* argv[]) {
	//SingleProcess();
	CrossProcess(argc, argv);
	return 0;
}