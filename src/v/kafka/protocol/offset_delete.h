/*
 * Copyright 2022 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once

#include "bytes/iobuf.h"
#include "kafka/protocol/errors.h"
#include "kafka/protocol/schemata/offset_delete_request.h"
#include "kafka/protocol/schemata/offset_delete_response.h"
#include "kafka/types.h"
#include "model/fundamental.h"
#include "model/timestamp.h"
#include "seastarx.h"

namespace kafka {

struct offset_delete_request final {
    using api_type = offset_delete_api;

    offset_delete_request_data data;

    // set during request processing after mapping group to ntp
    model::ntp ntp;

    void encode(response_writer& writer, api_version version) {
        data.encode(writer, version);
    }

    void decode(request_reader& reader, api_version version) {
        data.decode(reader, version);
    }

    friend std::ostream&
    operator<<(std::ostream& os, const offset_delete_request& r) {
        return os << r.data;
    }
};

struct offset_delete_response final {
    using api_type = offset_delete_api;

    offset_delete_response_data data;

    explicit offset_delete_response(
      offset_delete_request&, kafka::error_code error)
      : offset_delete_response(error) {}

    explicit offset_delete_response(kafka::error_code error) {
        data.error_code = error;
    }

    void encode(response_writer& writer, api_version version) {
        data.encode(writer, version);
    }

    void decode(iobuf buf, api_version version) {
        data.decode(std::move(buf), version);
    }

    friend std::ostream&
    operator<<(std::ostream& os, const offset_delete_response& r) {
        return os << r.data;
    }
};

} // namespace kafka
