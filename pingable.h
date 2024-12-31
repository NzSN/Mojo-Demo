#include "mojo/public/cpp/bindings/receiver.h"
#include "example/pingable.mojom.h"

namespace EXAMPLE_LOCAL {
	class PingableImpl : example::mojom::Pingable {
	public:
		explicit PingableImpl(
			mojo::PendingReceiver<example::mojom::Pingable> receiver, 
			scoped_refptr<base::SequencedTaskRunner> task_runner);
		~PingableImpl() override;
		PingableImpl(const PingableImpl&) = delete;
		PingableImpl& operator=(const PingableImpl&) = delete;

		void Ping(PingCallback callback) override;

		mojo::Receiver<example::mojom::Pingable> receiver_;
	};
}