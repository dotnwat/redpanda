#include "compat/raft_compat.h"

#include "raft/types.h"

/*
 *
 */
std::vector<raft::timeout_now_request>
compat_check<raft::timeout_now_request>::create_test_cases() {
    return {{}, {}};
}

void compat_check<raft::timeout_now_request>::to_json(
  raft::timeout_now_request, json::Writer<json::StringBuffer>&) {}

std::vector<compat_binary>
compat_check<raft::timeout_now_request>::to_binary(raft::timeout_now_request) {
    return {};
}

bool compat_check<raft::timeout_now_request>::check(
  json::Value&, std::vector<compat_binary>) {
    return false;
}

/*
 *
 */
std::vector<raft::timeout_now_reply>
compat_check<raft::timeout_now_reply>::create_test_cases() {
    return {{}};
}

void compat_check<raft::timeout_now_reply>::to_json(
  raft::timeout_now_reply, json::Writer<json::StringBuffer>&) {}

std::vector<compat_binary>
compat_check<raft::timeout_now_reply>::to_binary(raft::timeout_now_reply) {
    std::vector<compat_binary> b;
    iobuf bb;
    bb.append("asdf", 4);
    b.emplace_back(std::move(bb));
    return b;
}

bool compat_check<raft::timeout_now_reply>::check(
  json::Value&, std::vector<compat_binary>) {
    return false;
}
