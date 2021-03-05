/*
 * Copyright 2020 Vectorized, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */

#pragma once

#include "kafka/protocol/errors.h"
#include "kafka/protocol/schemata/produce_request.h"
#include "kafka/protocol/schemata/produce_response.h"
#include "kafka/protocol/kafka_batch_adapter.h"
#include "kafka/server/request_context.h"
#include "kafka/server/response.h"
#include "kafka/types.h"
#include "model/timestamp.h"
#include "seastarx.h"

#include <seastar/core/future.hh>

namespace kafka {

struct produce_response;

/**
 * Support starts at version 3 because this is the first version that supports
 * version 2 of the kafka message format.
 */
class produce_api final {
public:
    using response_type = produce_response;

    static constexpr const char* name = "produce";
    static constexpr api_key key = api_key(0);
};

struct produce_response;

struct produce_request final {
    using api_type = produce_api;

    produce_request_data data;

    void encode(response_writer& writer, api_version version) {
        data.encode(writer, version);
    }

    void decode(request_reader& reader, api_version version) {
        data.decode(reader, version);
    }

    /**
     * Build a generic error response for a given request.
     */
    produce_response make_error_response(error_code error) const;

    /// True if the request contains a batch with a transactional id.
    bool has_transactional = false;

    /// True if the request contains a batch with a producer id.
    bool has_idempotent = false;
};

std::ostream& operator<<(std::ostream&, const produce_request&);

struct produce_response final {
    using api_type = produce_api;

    struct partition {
        model::partition_id id;
        error_code error{kafka::error_code::none};
        model::offset base_offset{-1};
        model::timestamp log_append_time{-1};
        model::offset log_start_offset{-1}; // >= v5
    };

    struct topic {
        model::topic name;
        std::vector<partition> partitions;
    };

    std::vector<topic> topics;
    std::chrono::milliseconds throttle = std::chrono::milliseconds(0);

    void encode(const request_context& ctx, response& resp);
    void decode(iobuf buf, api_version version);
};

std::ostream& operator<<(std::ostream&, const produce_response&);

} // namespace kafka
