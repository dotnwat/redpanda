#include "redpanda/drain_manager.h"

#include "cluster/controller.h"
#include "cluster/controller_stm.h"
#include "cluster/partition_manager.h"
#include "vlog.h"

#include <seastar/core/smp.hh>

/*
 * cancel drain on stop
 * gate for rejecting new http requests on stop
 * need to block new groups from getting leadership (eg. right after making
 * consensus instance). maybe we can make this simple with a thread local flag
 */
drain_manager::drain_manager(
  ss::logger& log,
  cluster::controller* controller,
  ss::sharded<cluster::partition_manager>& partition_manager)
  : _log(log)
  , _controller(controller)
  , _partition_manager(partition_manager) {}

ss::future<> drain_manager::drain() {
    if (_drain.has_value()) {
        vlog(_log.info, "Draining is in progress");
        co_return;
    }

    vlog(_log.info, "Starting node drain...");

    _drain = do_drain().handle_exception([this](std::exception_ptr e) {
        vlog(_log.info, "Draining finished with error: {}", e);
    });
}

ss::future<> drain_manager::restore() {
    if (!_drain.has_value()) {
        vlog(_log.info, "No active draining");
        co_return;
    }

    // TODO: this isn't going to work because we'd need to call restore mutlple
    // ties
    //
    // we can let the handler for do drain clear _drain but we need to then
    // stick it ina shared ptr so that when we clear it we can avoid clear the
    // future in a cntinaton that defines the value for that future
    if (!_drain.value().available()) {
        if (!_abort.abort_requested()) {
            vlog(_log.info, "Stopping node drain...");
            _abort.request_abort();
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
    if (!_drain.has_value()) {
        return std::nullopt;
    }

    vlog(_log.info, "Fetching drain status");
    return drain_manager::drain_status{};
}

// Prevent explicit transfers
ss::future<> drain_manager::do_drain() {
    vlog(_log.info, "Draining...");

    /*
     * Prevent this node from becomming a leader for new and existing raft
     * groups. This does not immediately reliquish existing leadership.
     */
    if (ss::this_shard_id() == cluster::controller_stm_shard) {
        _controller->block_new_leadership();
    }

    for (const auto& p : _partition_manager.local().partitions()) {
        p.second->block_new_leadership();
    }

    /*
     * Transfer leadership for all raft groups away from this node. This
     * includes all kafka raft groups as well as the controller. This does not
     * apply to raft groups with one replica.
     *
     * synchronization: we arrange at the beginning of the drain process for new
     * and existing partitions to not acquire new leadership roles. thus here we
     * need only operate on a current snapshot of partitions in order to ensure
     * that leadership is drained for all partitions.
     */
    std::vector<ss::lw_shared_ptr<cluster::partition>> partitions;
    partitions.reserve(_partition_manager.local().partitions().size());
    for (const auto& p : _partition_manager.local().partitions()) {
        partitions.push_back(p.second);
        vlog(_log.info, "Draining partition: {}", partitions.back()->group());
    }

    // once leadership is transfered we don't need to worry about it anymoret
    // so we can iteratively shrink the set of groups that need to be
    // transferred

    co_return;
}

/*
 * TODO: need to make sure that when we unblock leadership that new partitions
 * don't race with the restore and get created in a blocked leadership state
 * and then aren't seen by this algorithm. sort of the reverse of the
 * syncronization needed when blocking leadership before draining existing.
 */
ss::future<> drain_manager::do_restore() {
    /*
     * Unblock this node from new leadership.
     */
    if (ss::this_shard_id() == cluster::controller_stm_shard) {
        _controller->unblock_new_leadership();
    }

    for (const auto& p : _partition_manager.local().partitions()) {
        p.second->unblock_new_leadership();
    }

    co_return;
}
