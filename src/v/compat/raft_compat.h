#pragma once

#include "compat/check.h"
#include "compat/json_helpers.h"
#include "raft/tests/random.h"
#include "raft/types.h"

namespace json {

inline void
rjson_serialize(json::Writer<json::StringBuffer>& w, const raft::vnode& v) {
    w.StartObject();
    w.Key("id");
    rjson_serialize(w, v.id());
    w.Key("revision");
    rjson_serialize(w, v.revision());
    w.EndObject();
}

inline void read_value(json::Value const& rd, raft::vnode& obj) {
    model::node_id node_id;
    model::revision_id revision;
    read_member(rd, "id", node_id);
    read_member(rd, "revision", revision);
    obj = raft::vnode(node_id, revision);
}

} // namespace json

/*
 * raft::timeout_now_request
 */
template<>
struct compat_check<raft::timeout_now_request> {
    static constexpr std::string_view name = "raft::timeout_now_request";

    static std::vector<raft::timeout_now_request> create_test_cases() {
        return tests::generate_instances<raft::timeout_now_request>();
    }

    static void to_json(
      raft::timeout_now_request obj, json::Writer<json::StringBuffer>& wr) {
        json_write(target_node_id);
        json_write(node_id);
        json_write(group);
        json_write(term);
    }

    static raft::timeout_now_request from_json(json::Value& rd) {
        raft::timeout_now_request obj;
        json_read(target_node_id);
        json_read(node_id);
        json_read(group);
        json_read(term);
        return obj;
    }

    static std::vector<compat_binary> to_binary(raft::timeout_now_request obj) {
        return compat_binary::serde_and_adl(obj);
    }

    static void check(raft::timeout_now_request obj, compat_binary test) {
        verify_adl_or_serde(obj, std::move(test));
    }
};

/*
 * raft::timeout_now_reply
 */
template<>
struct compat_check<raft::timeout_now_reply> {
    static constexpr std::string_view name = "raft::timeout_now_reply";

    static std::vector<raft::timeout_now_reply> create_test_cases() {
        return tests::generate_instances<raft::timeout_now_reply>();
    }

    static void
    to_json(raft::timeout_now_reply obj, json::Writer<json::StringBuffer>& wr) {
        json_write(target_node_id);
        json_write(term);
        json_write(result);
    }

    static raft::timeout_now_reply from_json(json::Value& rd) {
        raft::timeout_now_reply obj;
        json_read(target_node_id);
        json_read(term);
        auto result = json::read_member_enum(rd, "result", obj.result);
        switch (result) {
        case 0:
            obj.result = raft::timeout_now_reply::status::success;
            break;
        case 1:
            obj.result = raft::timeout_now_reply::status::failure;
            break;
        default:
            vassert(false, "invalid status: {}", result);
        }
        return obj;
    }

    static std::vector<compat_binary> to_binary(raft::timeout_now_reply obj) {
        return compat_binary::serde_and_adl(obj);
    }

    static void check(raft::timeout_now_reply obj, compat_binary test) {
        verify_adl_or_serde(obj, std::move(test));
    }
};
