/*
 * Copyright 2021 Vectorized, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once

#include "reflection/adl.h"
#include "serde/serde_exception.h"
#include "storage/index_state.h"
#include "storage/logger.h"
#include "vlog.h"

namespace storage::serde_compat {

struct index_state_serde {
    static constexpr int8_t ondisk_version = 3;

    static index_state decode(iobuf_parser& parser) {
        index_state retval;

        const auto size = reflection::adl<uint32_t>{}.from(parser);
        if (unlikely(parser.bytes_left() != size)) {
            throw serde::serde_exception(fmt_with_ctx(
              fmt::format,
              "Index size does not match header size. Got:{}, expected:{}",
              parser.bytes_left(),
              size));
        }

        const auto checksum = reflection::adl<uint64_t>{}.from(parser);
        retval.bitflags = reflection::adl<uint32_t>{}.from(parser);
        retval.base_offset = model::offset(
          reflection::adl<model::offset::type>{}.from(parser));
        retval.max_offset = model::offset(
          reflection::adl<model::offset::type>{}.from(parser));
        retval.base_timestamp = model::timestamp(
          reflection::adl<model::timestamp::type>{}.from(parser));
        retval.max_timestamp = model::timestamp(
          reflection::adl<model::timestamp::type>{}.from(parser));

        const uint32_t vsize = ss::le_to_cpu(
          reflection::adl<uint32_t>{}.from(parser));

        for (auto i = 0U; i < vsize; ++i) {
            retval.relative_offset_index.push_back(
              reflection::adl<uint32_t>{}.from(parser));
        }

        for (auto i = 0U; i < vsize; ++i) {
            retval.relative_time_index.push_back(
              reflection::adl<uint32_t>{}.from(parser));
        }

        for (auto i = 0U; i < vsize; ++i) {
            retval.position_index.push_back(
              reflection::adl<uint64_t>{}.from(parser));
        }

        retval.relative_offset_index.shrink_to_fit();
        retval.relative_time_index.shrink_to_fit();
        retval.position_index.shrink_to_fit();

        const auto computed_checksum = storage::index_state::checksum_state(
          retval);
        if (unlikely(checksum != computed_checksum)) {
            throw serde::serde_exception(fmt_with_ctx(
              fmt::format,
              "Invalid checksum for index. Got:{}, expected:{}",
              computed_checksum,
              checksum));
        }

        return retval;
    }

    /*
     * NOTE: taken by non-const because the size and checksum are updated.
     */
    static iobuf encode(index_state& st) {
        iobuf out;
        vassert(
          st.relative_offset_index.size() == st.relative_time_index.size()
            && st.relative_offset_index.size() == st.position_index.size(),
          "ALL indexes must match in size. {}",
          st);
        const uint32_t final_size
          = sizeof(uint64_t) // checksum
            + sizeof(storage::index_state::bitflags)
            + sizeof(storage::index_state::base_offset)
            + sizeof(storage::index_state::max_offset)
            + sizeof(storage::index_state::base_timestamp)
            + sizeof(storage::index_state::max_timestamp)
            + sizeof(uint32_t) // index size
            + (st.relative_offset_index.size() * (sizeof(uint32_t) * 2 + sizeof(uint64_t)));
        const uint64_t checksum = storage::index_state::checksum_state(st);
        reflection::serialize(
          out,
          ondisk_version,
          final_size,
          checksum,
          st.bitflags,
          st.base_offset(),
          st.max_offset(),
          st.base_timestamp(),
          st.max_timestamp(),
          uint32_t(st.relative_offset_index.size()));
        const uint32_t vsize = st.relative_offset_index.size();
        for (auto i = 0U; i < vsize; ++i) {
            reflection::adl<uint32_t>{}.to(out, st.relative_offset_index[i]);
        }
        for (auto i = 0U; i < vsize; ++i) {
            reflection::adl<uint32_t>{}.to(out, st.relative_time_index[i]);
        }
        for (auto i = 0U; i < vsize; ++i) {
            reflection::adl<uint64_t>{}.to(out, st.position_index[i]);
        }
        // add back the version and size field
        const auto expected_size = final_size + sizeof(int8_t)
                                   + sizeof(uint32_t);
        vassert(
          out.size_bytes() == expected_size,
          "Unexpected serialization size {} != expected {}",
          out.size_bytes(),
          expected_size);
        return out;
    }
};

}; // namespace storage::serde_compat
