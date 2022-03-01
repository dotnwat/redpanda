#include "redpanda/drain_manager.h"

#include "vlog.h"

drain_manager::drain_manager(
  ss::logger& log,
  cluster::controller* controller,
  ss::sharded<cluster::partition_manager>& partition_manager)
  : _log(log)
  , _controller(controller)
  , _partition_manager(partition_manager) {}

ss::future<> drain_manager::start_draining() {
    vlog(_log.info, "Starting node drain...");
    co_return;
}

ss::future<> drain_manager::stop_draining() {
    vlog(_log.info, "Stopping node drain...");
    co_return;
}

ss::future<drain_manager::drain_status> drain_manager::status() {
    vlog(_log.info, "Fetching drain status");
    co_return drain_manager::drain_status{};
}
