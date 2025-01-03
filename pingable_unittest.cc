#include <iostream>
#include <string>

#include <thread>
#include <chrono>

#include "pingable.h"
#include "base/threading/thread.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "base/test/task_environment.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"

#include "base/message_loop/message_pump.h"
#include "base/message_loop/message_pump_type.h"
#include "base/task/sequence_manager/sequence_manager_impl.h"

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

#define MAINTHREAD_SETUP \
	base::sequence_manager::SequenceManager::PrioritySettings priSettings(10, 0); \
	auto settings = base::sequence_manager::SequenceManager::Settings::Builder() \
						  .SetMessagePumpType(base::MessagePumpType::DEFAULT) \
						  .SetRandomisedSamplingEnabled(true) \
						  .SetAddQueueTimeToTasks(true) \
						  .SetPrioritySettings(std::move(priSettings)) \
						  .Build(); \
	auto seq = CreateSequenceManagerOnCurrentThreadWithPump( \
		base::MessagePump::Create(base::MessagePumpType::DEFAULT), std::move(settings)); \
	auto q = seq->CreateTaskQueue(base::sequence_manager::TaskQueue::Spec{QueueName::DEFAULT_TQ}); \
	seq->SetDefaultTaskRunner(q->task_runner());

using QueueName = ::perfetto::protos::pbzero::SequenceManagerTask::QueueName;

void InitializedProcess() {
	MAINTHREAD_SETUP;

	if (base::SequencedTaskRunner::HasCurrentDefault()) {
		std::cout << "Has default task runner" << std::endl;
	}
	else {
		std::cout << "No default task runner" << std::endl;
	}
}

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

void receiver_side() {
	MOJO_INITIALIZE;
	THREAD_INITIALIZE;
}

void remote_side() {
	mojo::core::Init();
	THREAD_INITIALIZE;
	
	constexpr base::TaskTraits default_traits = {};
	scoped_refptr<base::SequencedTaskRunner> runner = 
		base::ThreadPool::CreateSequencedTaskRunner(default_traits);
	mojo::Remote<example::mojom::Pingable> pingable;
	mojo::PendingReceiver<example::mojom::Pingable> receiver =
		pingable.BindNewPipeAndPassReceiver(runner);
	
	std::unique_ptr<EXAMPLE_LOCAL::PingableImpl> pingable_global = 
		std::make_unique<EXAMPLE_LOCAL::PingableImpl>(std::move(receiver), runner);
	pingable->Ping(base::BindOnce([](int a) { 
		std::cout << "Ping: " << a << std::endl;
	}));
}

void CrossProcess(int argc, char* argv[]) {
	if (argc == 1) {
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));

		std::string cmd{ "example_unittest.exe remote" };
		if (!CreateProcessA(
			argv[0], const_cast<char*>(cmd.c_str()), NULL,
			NULL, FALSE, 0,
			NULL, NULL, &si, &pi)) {
		
			std::cout << "Failed to create process" << std::endl;
		} else {
			receiver_side();

			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
		}
	} else if (std::string{argv[1]} == "remote") {
		remote_side();
	} else {
		std::cout << "No Match" << std::endl;
	}
}

int main(int argc, char* argv[]) {
	SingleProcess();
	//CrossProcess(argc, argv);
	return 0;
}