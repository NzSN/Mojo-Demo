#include <iostream>
#include <string>

#include "pingable.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "base/test/task_environment.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"

// Windows Specific headers
#include <windows.h>

#define MOJO_INITIALIZE		\
	mojo::core::Init();		\
	base::Thread ipc_thread("ipc!"); \
	ipc_thread.StartWithOptions( \
		base::Thread::Options(base::MessagePumpType::IO, 0)); \
	mojo::core::ScopedIPCSupport ipc_support( \
		ipc_thread.task_runner(), \
		mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);
#define THREAD_INITIALIZE \
	base::ThreadPoolInstance::Create("Default"); \
	auto param = base::ThreadPoolInstance::InitParams(10); \
	base::ThreadPoolInstance::Get()->Start(param);

void SingleProcess() {
	MOJO_INITIALIZE;
	THREAD_INITIALIZE;
	// Create Remote Process which will request ping 
	// to MainThread in MainProcess
	constexpr base::TaskTraits default_traits = {};
	scoped_refptr<base::SequencedTaskRunner> runner = 
		base::ThreadPool::CreateSequencedTaskRunner(default_traits);
	mojo::Remote<example::mojom::Pingable> pingable;
	mojo::PendingReceiver<example::mojom::Pingable> receiver =
		pingable.BindNewPipeAndPassReceiver(runner);
	
	std::unique_ptr<EXAMPLE_LOCAL::PingableImpl> pingable_global = 
		std::make_unique<EXAMPLE_LOCAL::PingableImpl>(std::move(receiver), runner);
	pingable_global->Ping(base::BindOnce([](int a) { 
		std::cout << "Ping: " << a << std::endl;
	}));

	pingable.reset();
}

void CrossProcess(int argc, char* argv[]) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (std::)

	const auto mainThread = [] {
		MOJO_INITIALIZE;
		THREAD_INITIALIZE;
	};

	if (!CreateProcess(
		NULL, argv[0], NULL, 
		NULL, FALSE,   0,
		NULL, NULL, &si, &pi)) {

	}

	mainThread();
		

}

int main(int argc, char* argv[]) {
	CrossProcess();
	return 0;
}