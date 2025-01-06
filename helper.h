

using QueueName = ::perfetto::protos::pbzero::SequenceManagerTask::QueueName;

#define MOJO_BASIC_INITIALIZE(C)  mojo::core::Init(C)
#define MOJO_INITIALIZE(T)		\
	mojo::core::Configuration config;\
	config.is_broker_process = T;\
	MOJO_BASIC_INITIALIZE(config); \
	mojo::core::ScopedIPCSupport ipc_support( \
		base::SingleThreadTaskRunner::GetCurrentDefault(), \
		mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

#define THREAD_INITIALIZE {\
	base::ThreadPoolInstance::Create("Default"); \
	auto param = base::ThreadPoolInstance::InitParams(10); \
	base::ThreadPoolInstance::Get()->Start(param); \
}

#define MAINTHREAD_SETUP \
	base::sequence_manager::SequenceManager::PrioritySettings __priSettings(10, 0); \
	auto __settings = base::sequence_manager::SequenceManager::Settings::Builder() \
						  .SetMessagePumpType(base::MessagePumpType::IO) \
						  .Build(); \
	auto __seqM = CreateSequenceManagerOnCurrentThreadWithPump( \
		base::MessagePump::Create(base::MessagePumpType::IO), std::move(__settings)); \
	auto __q = __seqM->CreateTaskQueue(base::sequence_manager::TaskQueue::Spec{QueueName::DEFAULT_TQ}); \
	__seqM->SetDefaultTaskRunner(__q->task_runner());
