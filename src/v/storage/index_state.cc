// Copyright 2020 Vectorized, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "storage/index_state.h"

#include "bytes/iobuf_parser.h"
#include "bytes/utils.h"
#include "likely.h"
#include "model/timestamp.h"
#include "reflection/adl.h"
#include "serde/serde.h"
#include "serde/serde_exception.h"
#include "storage/index_state_serde_compat.h"
#include "storage/logger.h"
#include "vassert.h"
#include "vlog.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <optional>

namespace storage {

bool index_state::maybe_index(
  size_t accumulator,
  size_t step,
  size_t starting_position_in_file,
  model::offset batch_base_offset,
  model::offset batch_max_offset,
  model::timestamp first_timestamp,
  model::timestamp last_timestamp) {
    vassert(
      batch_base_offset >= base_offset,
      "cannot track offsets that are lower than our base, o:{}, "
      "_state.base_offset:{} - index: {}",
      batch_base_offset,
      base_offset,
      *this);

    bool retval = false;
    // index_state
    if (empty()) {
        base_timestamp = first_timestamp;
        max_timestamp = first_timestamp;
        retval = true;
    }
    // NOTE: we don't need the 'max()' trick below because we controll the
    // offsets ourselves and it would be a bug otherwise - see assert above
    max_offset = batch_max_offset;
    // some clients leave max timestamp uninitialized in cases there is a
    // single record in a batch in this case we use first timestamp as a
    // last one
    last_timestamp = std::max(first_timestamp, last_timestamp);
    max_timestamp = std::max(max_timestamp, last_timestamp);
    // always saving the first batch simplifies a lot of book keeping
    if (accumulator >= step || retval) {
        // We know that a segment cannot be > 4GB
        add_entry(
          batch_base_offset() - base_offset(),
          last_timestamp() - base_timestamp(),
          starting_position_in_file);

        retval = true;
    }
    return retval;
}

std::ostream& operator<<(std::ostream& o, const index_state& s) {
    return o << "{header_bitflags:" << s.bitflags
             << ", base_offset:" << s.base_offset
             << ", max_offset:" << s.max_offset
             << ", base_timestamp:" << s.base_timestamp
             << ", max_timestamp:" << s.max_timestamp << ", index("
             << s.relative_offset_index.size() << ","
             << s.relative_time_index.size() << "," << s.position_index.size()
             << ")}";
}

std::optional<index_state> index_state::hydrate_from_buffer(iobuf b) {
    iobuf_parser parser(std::move(b));

    auto version = reflection::adl<int8_t>{}.from(parser);
    switch (version) {
    case serde_compat::index_state_serde::ondisk_version:
        break;

    default:
        /*
         * v3: changed the on-disk format to use 64-bit values for physical
         * offsets to avoid overflow for segments larger than 4gb. backwards
         * compat would require converting the overflowed values. instead, we
         * fully deprecate old versions and rebuild the offset indexes.
         *
         * v2: fully deprecated
         *
         * v1: fully deprecated
         *
         *     version 1 code stored an on disk size that was calculated as 4
         *     bytes too small, and the decoder did not check the size. instead
         *     of rebuilding indexes for version 1 we'll adjust the size because
         *     the checksums are still verified.
         *
         * v0: fully deprecated
         */
        vlog(
          stlog.debug,
          "Forcing index rebuild for unknown or unsupported version {}",
          version);
        return std::nullopt;
    }

    try {
        return serde_compat::index_state_serde::decode(parser);
    } catch (const serde::serde_exception& ex) {
        vlog(stlog.debug, "Decoding index state: {}", ex.what());
        return std::nullopt;
    }
}

iobuf index_state::checksum_and_serialize() {
    return serde_compat::index_state_serde::encode(*this);
}

void read_nested(
  iobuf_parser& in, index_state& st, const size_t bytes_left_limit) {
    if (
      serde::peek_version(in)
      > serde_compat::index_state_serde::ondisk_version) {
        using serde::read_nested;
        const auto hdr = serde::read_header<index_state>(in, bytes_left_limit);

        iobuf tmp;
        uint32_t tmp_crc = 0;
        read_nested(in, tmp, hdr._bytes_left_limit);
        read_nested(in, tmp_crc, hdr._bytes_left_limit);

        crc::crc32c crc;
        crc_extend_iobuf(crc, tmp);
        const uint32_t expected_tmp_crc = crc.value();

        if (tmp_crc != expected_tmp_crc) {
            throw serde::serde_exception(fmt_with_ctx(
              fmt::format,
              "Mistmatched checksum {} expected {}",
              tmp_crc,
              expected_tmp_crc));
        }

        iobuf_parser p(std::move(tmp));
        st.bitflags = read_nested<decltype(st.bitflags)>(p, 0U);
        st.base_offset = read_nested<decltype(st.base_offset)>(p, 0U);
        st.max_offset = read_nested<decltype(st.max_offset)>(p, 0U);
        st.base_timestamp = read_nested<decltype(st.base_timestamp)>(p, 0U);
        st.max_timestamp = read_nested<decltype(st.max_timestamp)>(p, 0U);
        st.relative_offset_index
          = read_nested<decltype(st.relative_offset_index)>(p, 0U);
        st.relative_time_index = read_nested<decltype(st.relative_time_index)>(
          p, 0U);
        st.position_index = read_nested<decltype(st.position_index)>(p, 0U);

        return;
    }

    /*
     * supported old version to avoid rebuilding all indices.
     */
    const auto version = reflection::adl<int8_t>{}.from(in);
    if (version == serde_compat::index_state_serde::ondisk_version) {
        st = serde_compat::index_state_serde::decode(in);
        return;
    }

    /*
     * unsupported old version. the call will rebuild the index.
     */
    vlog(stlog.debug, "Rebuilding index for version {}", version);
    throw serde::serde_exception(
      fmt_with_ctx(fmt::format, "Unsupported version: {}", version));
}

void index_state::serde_write(iobuf& out) const {
    using serde::write;

    iobuf tmp;
    write(tmp, bitflags);
    write(tmp, base_offset);
    write(tmp, max_offset);
    write(tmp, base_timestamp);
    write(tmp, max_timestamp);
    write(tmp, relative_offset_index.copy());
    write(tmp, relative_time_index.copy());
    write(tmp, position_index.copy());

    crc::crc32c crc;
    crc_extend_iobuf(crc, tmp);
    const uint32_t tmp_crc = crc.value();

    write(out, std::move(tmp));
    write(out, tmp_crc);
}

} // namespace storage
