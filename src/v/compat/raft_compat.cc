#include "compat/raft_compat.h"

#include "compat/json_helpers.h"
#include "raft/tests/random.h"
#include "raft/types.h"

#define json_write(_fname) json::write_member(wr, #_fname, obj._fname)
#define json_read(_fname) json::read_member(rd, #_fname, obj._fname)

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
 *
 */
std::vector<raft::timeout_now_request>
compat_check<raft::timeout_now_request>::create_test_cases() {
    using generator = instance_generator<raft::timeout_now_request>;
    auto res = generator::limits();
    res.push_back(generator::random());
    return res;
}

void compat_check<raft::timeout_now_request>::to_json(
  raft::timeout_now_request obj, json::Writer<json::StringBuffer>& wr) {
    json_write(target_node_id);
    json_write(node_id);
    json_write(group);
    json_write(term);
}

raft::timeout_now_request
compat_check<raft::timeout_now_request>::from_json(json::Value& rd) {
    raft::timeout_now_request obj;
    {
        json_read(target_node_id);
        json_read(node_id);
        json_read(group);
        json_read(term);
    }
    return obj;
}

std::vector<compat_binary> compat_check<raft::timeout_now_request>::to_binary(
  raft::timeout_now_request obj) {
    return {
      compat_binary::serde(obj),
      compat_binary("adl", reflection::to_iobuf(std::move(obj))),
    };
}

void compat_check<raft::timeout_now_request>::check(
  raft::timeout_now_request obj, compat_binary test) {
    if (test.name == "adl") {
        auto tmp = reflection::from_iobuf<raft::timeout_now_request>(
          std::move(test.data));
        vassert(obj == tmp, "not equal");
    } else if (test.name == "serde") {
        auto tmp = serde::from_iobuf<raft::timeout_now_request>(
          std::move(test.data));
        vassert(obj == tmp, "not equal");
    } else {
        vassert(false, "unknown type {}", test.name);
    }
}

/*
 *
 */
std::vector<raft::timeout_now_reply>
compat_check<raft::timeout_now_reply>::create_test_cases() {
    using generator = instance_generator<raft::timeout_now_reply>;
    auto res = generator::limits();
    res.push_back(generator::random());
    return res;
}

void compat_check<raft::timeout_now_reply>::to_json(
  raft::timeout_now_reply obj, json::Writer<json::StringBuffer>& wr) {
    json_write(target_node_id);
    json_write(term);
    json_write(result);
}

raft::timeout_now_reply
compat_check<raft::timeout_now_reply>::from_json(json::Value& rd) {
    raft::timeout_now_reply obj;
    {
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
    }
    return obj;
}

std::vector<compat_binary>
compat_check<raft::timeout_now_reply>::to_binary(raft::timeout_now_reply obj) {
    return {
      compat_binary::serde(obj),
      compat_binary("adl", reflection::to_iobuf(std::move(obj))),
    };
}

void compat_check<raft::timeout_now_reply>::check(
  raft::timeout_now_reply obj, compat_binary test) {
    if (test.name == "adl") {
        auto tmp = reflection::from_iobuf<raft::timeout_now_reply>(
          std::move(test.data));
        vassert(obj == tmp, "not equal");
    } else if (test.name == "serde") {
        auto tmp = serde::from_iobuf<raft::timeout_now_reply>(
          std::move(test.data));
        vassert(obj == tmp, "not equal");
    } else {
        vassert(false, "unknown type {}", test.name);
    }
}
