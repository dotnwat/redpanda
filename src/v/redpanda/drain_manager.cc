#include "redpanda/drain_manager.h"

drain_manager::drain_manager(
  cluster::controller* controller,
  ss::sharded<cluster::partition_manager>& partition_manager)
  : _controller(controller)
  , _partition_manager(partition_manager) {}
