#include "redpanda/drain_manager.h"

#include "cluster/partition_manager.h"
#include "random/generators.h"
#include "vlog.h"

#include <seastar/core/lowres_clock.hh>
#include <seastar/core/smp.hh>
#include <seastar/core/when_all.hh>

drain_manager::drain_manager(
  ss::logger& log, ss::sharded<cluster::partition_manager>& partition_manager)
  : _log(log)
  , _partition_manager(partition_manager) {}

ss::future<> drain_manager::start() {
    vassert(!_drain.has_value(), "service cannot be restarted");
    vlog(_log.info, "Drain manager starting");
    _drain = task();
    co_return;
}

ss::future<> drain_manager::stop() {
    vassert(_drain.has_value(), "service has not been started");
    vlog(_log.info, "Drain manager stopping");
    _stop = true;
    _sem.signal();
    return _drain.value().handle_exception([this](std::exception_ptr e) {
        vlog(_log.warn, "Draining manager task experience error: {}", e);
    });
}

ss::future<> drain_manager::drain() {
    if (_stop) {
        // handle http requests racing with shutdown
        co_return;
    }

    if (_draining) {
        vlog(_log.info, "Node draining is already active");
        co_return;
    }

    vlog(_log.info, "Node draining is starting");
    _draining = true;
    _sem.signal();
}

ss::future<> drain_manager::restore() {
    if (_stop) {
        co_return;
    }

    if (!_draining) {
        vlog(_log.info, "Node draining is not active");
        co_return;
    }

    vlog(_log.info, "Node draining is stopping");
    _draining = false;
    _sem.signal();
}

// TODO
std::optional<drain_manager::drain_status> drain_manager::status() {
    if (_stop) {
        return drain_manager::drain_status{};
    }

    if (!_drain.has_value()) {
        return std::nullopt;
    }
    vlog(_log.info, "Fetching drain status");
    return drain_manager::drain_status{};
}

ss::future<> drain_manager::task() {
    while (true) {
        co_await _sem.wait();
        _sem.consume(_sem.available_units());

        if (_stop) {
            break;
        }

        const auto draining = _draining;
        try {
            if (draining) {
                co_await do_drain();
            } else {
                co_await do_restore();
            }
        } catch (...) {
            vlog(
              _log.warn,
              "Draining task {{{}}} experienced error: {}",
              draining ? "drain" : "restore",
              std::current_exception());
        }
    }
}

ss::future<> drain_manager::do_drain() {
    vlog(_log.info, "Node draining has started");

    /*
     * Prevent this node from becomming a leader for new and existing raft
     * groups. This does not immediately reliquish existing leadership. it is
     * assumed that all raft groups (e.g. controller/raft0 and kafka data) are
     * represented in the partition manager.
     */
    _partition_manager.local().block_new_leadership();

    for (const auto& p : _partition_manager.local().partitions()) {
        p.second->block_new_leadership();
    }

    while (_draining && !_stop) {
        /*
         * build a set of eligible partitions. ignore any raft groups that that
         * will fail when transferring leadership and which shouldn't be
         * retried. note that above when we block new leadership we don't bother
         * skipping over groups without followers. this is safe because such a
         * group won't lose leadership in the first place.
         */
        std::vector<ss::lw_shared_ptr<cluster::partition>> eligible;
        eligible.reserve(_partition_manager.local().partitions().size());
        for (const auto& p : _partition_manager.local().partitions()) {
            if (!p.second->is_leader() || !p.second->has_followers()) {
                continue;
            }
            eligible.push_back(p.second);
        }

        if (eligible.empty()) {
            break;
        }

        /*
         * choose a random sample from the set of eligible partitions. this is
         * useful when we have a draining policy in which we want to drain as
         * much as possible even if some groups continue to have leadership
         * transfer errors. an alternative approach would be to fence off groups
         * experiencing errors, but then we would have to create some type of
         * retry policy to deal with those partitions.
         */
        std::vector<ss::lw_shared_ptr<cluster::partition>> selected;
        selected.reserve(max_parallel_transfers);
        std::sample(
          eligible.begin(),
          eligible.end(),
          std::back_inserter(selected),
          max_parallel_transfers,
          random_generators::internal::gen);
        eligible.clear();

        /*
         * start a group of transfers
         */
        std::vector<ss::future<std::error_code>> transfers;
        transfers.reserve(selected.size());
        for (auto& p : selected) {
            transfers.push_back(p->transfer_leadership(std::nullopt));
        }

        vlog(_log.info, "Draining leadership from {} groups", transfers.size());

        auto started = ss::lowres_clock::now();

        auto results = co_await ss::when_all(
          transfers.begin(), transfers.end());

        size_t failed = 0;
        for (auto& f : results) {
            try {
                auto err = f.get0();
                if (err) {
                    vlog(
                      _log.debug,
                      "Draining leadership failed for group: {}",
                      err);
                    failed++;
                }
            } catch (...) {
                vlog(
                  _log.debug,
                  "Draining leadership failed for group: {}",
                  std::current_exception());
                failed++;
            }
        }

        vlog(
          _log.info,
          "Draining leadership from {} groups {} succeeded",
          transfers.size(),
          transfers.size() - failed);

        /*
         * to avoid spinning, cool off if we failed fast
         */
        auto dur = ss::lowres_clock::now() - started;
        if (failed > 0 && dur < transfer_throttle && _draining && !_stop) {
            co_await ss::sleep(transfer_throttle - dur);
        }
    }

    vlog(_log.info, "Node draining has completed");
}

/*
 * Unblock this node from new leadership.
 *
 * Currently the unblocking process does not attempt to restore leadership back
 * to the node. This is assumed to be handled at a higher level (e.g. by the
 * operator by enabling or poking the cluster leadership rebalancer). However,
 * we could imagine being more aggresive here in the future.
 */
ss::future<> drain_manager::do_restore() {
    vlog(_log.info, "Node drain stopped");

    _partition_manager.local().unblock_new_leadership();

    for (const auto& p : _partition_manager.local().partitions()) {
        p.second->unblock_new_leadership();
    }

    co_return;
}
