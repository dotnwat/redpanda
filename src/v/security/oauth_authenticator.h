/*
 * Copyright 2020 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once
#include "security/acl.h"
#include "security/sasl_authentication.h"

namespace security {

class oauth_authenticator final : public sasl_mechanism {
public:
    static constexpr const char* name = "OAUTHBEARER";

    result<bytes> authenticate(bytes_view auth_bytes) override;
    bool complete() const override { return false; }
    bool failed() const override { return false; }

    const acl_principal& principal() const override { return _principal; }

private:
    acl_principal _principal;
};

} // namespace security
