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

int main(int argc, char* argv[]) {
	base::CommandLine::Init(argc, argv);

	MAINTHREAD_SETUP;
	MOJO_INITIALIZE(false);
	
	mojo::IncomingInvitation invitation =
		mojo::IncomingInvitation::Accept(
		  mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
		    *base::CommandLine::ForCurrentProcess()));
	assert(invitation.is_valid());
	mojo::ScopedMessagePipeHandle pipe =
		invitation.ExtractMessagePipe("x");
	assert(pipe.is_valid());
	
	mojo::PendingRemote<example::mojom::Pingable> pingable_pending{
		std::move(pipe), 0 };
	mojo::Remote<example::mojom::Pingable> pingable{
		std::move(pingable_pending) };

	pingable->Ping(base::BindOnce([](int a) {
		std::cout << "Ping: " << a << std::endl;
	}));

	base::RunLoop run_loop;
	base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](base::OnceClosure quit_closure) {
			std::cout << "'Renderer process' shutting down" << std::endl;
			std::move(quit_closure).Run();
          },
          run_loop.QuitClosure()),
      base::Seconds(1));

	run_loop.Run();

	return 0;
}