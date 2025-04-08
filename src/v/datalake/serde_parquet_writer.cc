#include "datalake/serde_parquet_writer.h"

#include "base/vlog.h"
#include "config/configuration.h"
#include "config/node_config.h"
#include "datalake/logger.h"
#include "datalake/schema_parquet.h"
#include "datalake/values_parquet.h"
#include "utils/directory_walker.h"
#include "utils/human.h"
#include "version/version.h"

#include <seastar/core/seastar.hh>
#include <seastar/util/defer.hh>

namespace datalake {

#if 0
static ss::future<uint64_t> disk_usage() {
    const auto path = config::node().datalake_staging_path();

    if (!co_await ss::file_exists(path.string())) {
        co_return 0;
    }

    chunked_vector<std::filesystem::path> files;
    co_await directory_walker::walk(
      path.string(), [&files, path](const ss::directory_entry& de) {
          if (de.type == ss::directory_entry_type::regular) {
              files.push_back(path / std::filesystem::path(de.name));
          }
          return ss::now();
      });

    uint64_t total = 0;
    co_await ss::max_concurrent_for_each(
      files.begin(),
      files.end(),
      config::shard_local_cfg().space_management_max_log_concurrency(),
      [&total](const std::filesystem::path& path) {
          return ss::file_size(path.string())
            .then([&total](uint64_t size) { total += size; })
            .handle_exception_type(
              [path](const std::filesystem::filesystem_error& e) {
                  if (e.code() == std::errc::no_such_file_or_directory) {
                      vlog(
                        datalake_log.debug,
                        "Stat failed for path: {}: {}",
                        path,
                        e.code());
                  }
                  return ss::make_exception_future<>(e);
              })
            .handle_exception([path](std::exception_ptr eptr) {
                vlog(
                  datalake_log.warn,
                  "Stat failed for path: {}: {}",
                  path,
                  eptr);
            });
      });

    co_return total;
}
#endif

/*
 * TODO when the multiplexer flushes it needs to check if any errors during
 * processing were recoverable, like oom, in which case flushing proceeds. so if
 * we add new error codes for disk usage make sure we handle that case.
 */
ss::future<writer_error> serde_parquet_writer::add_data_struct(
  iceberg::struct_value value, size_t, ss::abort_source& as) {
    auto conversion_result = co_await to_parquet_value(
      std::make_unique<iceberg::struct_value>(std::move(value)));
    if (conversion_result.has_error()) {
        co_return writer_error::parquet_conversion_error;
    }

    auto group = std::get<serde::parquet::group_value>(
      std::move(conversion_result.value()));
    try {
        auto stats = co_await _writer.write_row(std::move(group));
        auto stats_updater = ss::defer([this, stats] {
            _buffered_bytes = stats.buffered_size;
            _flushed_bytes = stats.flushed_size;
        });

        // vlog(
        //   datalake_log.info,
        //   "XXX: buffered {} flushed {} total {} - ondisk total {}",
        //   human::bytes(_buffered_bytes),
        //   human::bytes(_flushed_bytes),
        //   human::bytes(_buffered_bytes + _flushed_bytes),
        //   human::bytes(co_await disk_usage()));

        /*
         * handle memory reservation
         */
        auto new_buffered_bytes = stats.buffered_size;
        if (new_buffered_bytes > _buffered_bytes) {
            auto reservation_result = co_await _mem_tracker.reserve_bytes(
              new_buffered_bytes - _buffered_bytes, as);
            if (reservation_result != reservation_error::ok) {
                co_return map_to_writer_error(reservation_result);
            }
        } else if (new_buffered_bytes < _buffered_bytes) {
            // underlying writer may choose to compress data when
            // a page worth of data is batched, at which point the
            // resulting compressed size is smaller than before and
            // allows us to free up some bytes.
            co_await _mem_tracker.free_bytes(
              _buffered_bytes - new_buffered_bytes, as);
        }

        /*
         * handle disk reservation
         *
         * why it accounts for buffered plus flushed
         */
        const auto total_bytes = _buffered_bytes + _flushed_bytes;
        const auto new_total_bytes = stats.buffered_size + stats.flushed_size;
        if (new_total_bytes > total_bytes) {
            auto reservation_result = co_await _mem_tracker.reserve_disk_bytes(
              new_total_bytes - total_bytes, as);
            if (reservation_result != reservation_error::ok) {
                co_return map_to_writer_error(reservation_result);
            }
        } else if (new_total_bytes < total_bytes) {
            /*
             * the amount of data on disk won't shrink, but the total we are
             * working with here includes data buffered in memory w
             */
            co_await _mem_tracker.free_disk_bytes(
              total_bytes - new_total_bytes, as);
        }
    } catch (...) {
        vlog(
          datalake_log.warn,
          "Error writing parquet row - {}",
          std::current_exception());
        co_return writer_error::file_io_error;
    }
    co_return writer_error::ok;
}

size_t serde_parquet_writer::buffered_bytes() const { return _buffered_bytes; }
size_t serde_parquet_writer::flushed_bytes() const { return _flushed_bytes; }

ss::future<> serde_parquet_writer::flush() {
    co_await _writer.flush_row_group();
    auto stats = _writer.stats();
    _buffered_bytes = stats.buffered_size;
    _flushed_bytes = stats.flushed_size;
    vassert(
      _buffered_bytes == 0,
      "Memory buffered in the writer after flush: {}",
      _buffered_bytes);
}

ss::future<writer_error> serde_parquet_writer::finish() {
    co_await _writer.close();
    _buffered_bytes = _flushed_bytes = 0;
    co_return writer_error::ok;
}

ss::future<std::unique_ptr<parquet_ostream>>
serde_parquet_writer_factory::create_writer(
  const iceberg::struct_type& schema,
  ss::output_stream<char> out,
  writer_mem_tracker& mem_tracker) {
    serde::parquet::writer::options opts{
      .schema = schema_to_parquet(schema),
      .version = ss::sstring(redpanda_git_version()),
      .build = ss::sstring(redpanda_git_revision()),
      .compress = true,
    };
    serde::parquet::writer writer(std::move(opts), std::move(out));
    co_await writer.init();
    co_return std::make_unique<serde_parquet_writer>(
      std::move(writer), mem_tracker);
}

} // namespace datalake
