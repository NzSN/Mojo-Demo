#include <iostream>
#include <string>

#include <cassert>
#include <thread>
#include <chrono>

#include "base/test/bind.h"

#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel_server.h"

#include "base/process/launch.h"
#include "base/threading/thread.h"
#include "base/test/task_environment.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/message_loop/message_pump.h"
#include "base/message_loop/message_pump_type.h"
#include "base/task/sequence_manager/sequence_manager_impl.h"

// Windows Specific headers
#include <windows.h>

#include "pingable.h"
#include "helper.h"

void SingleProcess() {
	MAINTHREAD_SETUP;
	MOJO_INITIALIZE(true);

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

void BrowserMain(int argc, char* argv[]) {
	MAINTHREAD_SETUP;
	MOJO_INITIALIZE(true);

	mojo::PlatformChannel channel;

	mojo::OutgoingInvitation invitation;
	mojo::ScopedMessagePipeHandle pipe =
		invitation.AttachMessagePipe("x");
	assert(pipe.is_valid());

	// Spawn Remote-Process	
	base::LaunchOptions options;
	base::CommandLine command_line =
		base::CommandLine::FromString(std::wstring{ L"./renderer" });
	channel.PrepareToPassRemoteEndpoint(&options, &command_line);

	base::Process remote_process = base::LaunchProcess(command_line, options);
	channel.RemoteProcessLaunchAttempted();

	mojo::OutgoingInvitation::Send(
		std::move(invitation),
		remote_process.Handle(),
		channel.TakeLocalEndpoint(),
		base::BindRepeating([](const std::string& str) {
			std::cout << str << std::endl;
		}));

	mojo::PendingReceiver<example::mojom::Pingable> receiver_pending{ 
		std::move(pipe) };
	std::unique_ptr<EXAMPLE_LOCAL::PingableImpl> pingable_global = 
		std::make_unique<EXAMPLE_LOCAL::PingableImpl>(std::move(receiver_pending));

	base::RunLoop run_loop;
	base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](base::OnceClosure quit_closure) {
			std::cout << "'Browser process' shutting down" << std::endl;
			std::move(quit_closure).Run();
          },
          run_loop.QuitClosure()),
      base::Seconds(1));

	run_loop.Run();
}

int main(int argc, char* argv[]) {
	BrowserMain(argc, argv);
	return 0;
}