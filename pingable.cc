#include <iostream>
#include "pingable.h"

namespace EXAMPLE_LOCAL {

PingableImpl::PingableImpl(mojo::PendingReceiver<example::mojom::Pingable> receiver)
	: receiver_(this, std::move(receiver)) {}

PingableImpl::PingableImpl(
	mojo::PendingReceiver<example::mojom::Pingable> receiver,
	scoped_refptr<base::SequencedTaskRunner> task_runner)
	: receiver_(this, std::move(receiver), task_runner) {}

PingableImpl::~PingableImpl() {
	receiver_.reset();
}


void PingableImpl::Ping(PingCallback callback) {
	std::move(callback).Run(5);
}

}