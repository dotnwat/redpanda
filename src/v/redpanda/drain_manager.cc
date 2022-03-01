#include "redpanda/drain_manager.h"

#include "cluster/controller.h"
#include "cluster/controller_stm.h"
#include "cluster/partition_manager.h"
#include "vlog.h"

#include <seastar/core/smp.hh>

static void verify_core() {
    vassert(
      ss::this_shard_id() == cluster::controller_stm_shard,
      "Drain manager invoked on unexpected core {} != {}",
      ss::this_shard_id(),
      cluster::controller_stm_shard);
}

/*
 * cancel drain on stop
 */
drain_manager::drain_manager(
  ss::logger& log,
  cluster::controller* controller,
  ss::sharded<cluster::partition_manager>& partition_manager)
  : _log(log)
  , _controller(controller)
  , _partition_manager(partition_manager) {}

ss::future<> drain_manager::drain() {
    verify_core();

    if (_drain.has_value()) {
        // in progress, succeeded, failed
        vlog(_log.info, "Draining is in progress");
        co_return;
    }

    vlog(_log.info, "Starting node drain...");
    _drain = do_drain().handle_exception([this](std::exception_ptr e) {
        vlog(_log.info, "Draining finished with error: {}", e);
    });
}

ss::future<> drain_manager::restore() {
    verify_core();

    if (!_drain.has_value()) {
        vlog(_log.info, "No active draining");
        co_return;
    }

    if (!_drain.value().available()) {
        if (!_abort.abort_requested()) {
            _abort.request_abort();
            vlog(_log.info, "Stopping node drain...");
        }
        co_return;
    }

    /*
     * drain has finished and leadership on this node may be unblocked. this
     * happens even if an error ocurred to account for a partial drain.
     */
    co_await do_restore();

    /*
     * since _drain exceptions are handled this shouldn't happen. handle
     * defensively to avoid unexpected unhandled exceptional future errors.
     */
    if (_drain.value().failed()) {
        vlog(
          _log.info,
          "Draining finished with error: {}",
          _drain.value().get_exception());
    }

    // reset drain manager state machine
    _drain.reset();
    _abort = ss::abort_source{};
}

std::optional<drain_manager::drain_status> drain_manager::status() {
    verify_core();

    if (!_drain.has_value()) {
        return std::nullopt;
    }

    vlog(_log.info, "Fetching drain status");
    return drain_manager::drain_status{};
}

ss::future<> drain_manager::drain_leadership(cluster::partition_manager& pm) {
    /*
     * synchronization: we arrange at the beginning of the drain process for new
     * and existing partitions to not acquire new leadership roles. thus here we
     * need only operate on a current snapshot of partitions in order to ensure
     * that leadership is drained for all partitions.
     */
    std::vector<ss::lw_shared_ptr<cluster::partition>> partitions;
    partitions.reserve(pm.partitions().size());
    for (const auto& p : pm.partitions()) {
        partitions.push_back(p.second);
        vlog(_log.info, "Draining partition: {}", partitions.back()->group());
    }

    co_return;
}

ss::future<> drain_manager::do_drain() {
    vlog(_log.info, "Draining...");

    /*
     * Prevent this node from becomming a leader for new and existing raft
     * groups. This does not immediately reliquish existing leadership.
     */
    _controller->block_new_leadership();
    co_await _partition_manager.invoke_on_all(
      [](cluster::partition_manager& pm) {
          for (const auto& p : pm.partitions()) {
              p.second->block_new_leadership();
          }
      });

    /*
     * Transfer leadership for all raft groups away from this node. This
     * includes all kafka raft groups as well as the controller. This does not
     * apply to raft groups with one replica.
     */
    co_await _partition_manager.invoke_on_all(
      [this](auto& pm) { return drain_leadership(pm); });

    co_return;
}

ss::future<> drain_manager::do_restore() {
    /*
     * Unblock this node from new leadership.
     */
    _controller->unblock_new_leadership();
    co_await _partition_manager.invoke_on_all(
      [](cluster::partition_manager& pm) {
          for (const auto& p : pm.partitions()) {
              p.second->unblock_new_leadership();
          }
      });
}
