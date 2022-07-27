#pragma once

#include "cluster/metadata_dissemination_types.h"
#include "cluster/tests/random.h"
#include "compat/check.h"
#include "compat/json_helpers.h"
#include "model/fundamental.h"

namespace json {

inline void rjson_serialize(
  json::Writer<json::StringBuffer>& w, const cluster::ntp_leader& v) {
    w.StartObject();
    w.Key("ntp");
    rjson_serialize(w, v.ntp);
    w.Key("term");
    rjson_serialize(w, v.term);
    w.Key("leader_id");
    rjson_serialize(w, v.leader_id);
    w.EndObject();
}

inline void read_value(json::Value const& rd, cluster::ntp_leader& obj) {
    read_member(rd, "ntp", obj.ntp);
    read_member(rd, "term", obj.term);
    read_member(rd, "leader_id", obj.leader_id);
}

} // namespace json

/*
 * cluster::update_leadership_request
 */
template<>
struct compat_check<cluster::update_leadership_request> {
    static constexpr std::string_view name
      = "cluster::update_leadership_request";

    static std::vector<cluster::update_leadership_request> create_test_cases() {
        return tests::generate_instances<cluster::update_leadership_request>();
    }

    static void to_json(
      cluster::update_leadership_request obj,
      json::Writer<json::StringBuffer>& wr) {
        json_write(leaders);
    }

    static cluster::update_leadership_request from_json(json::Value& rd) {
        cluster::update_leadership_request obj;
        json_read(leaders);
        return obj;
    }

    static std::vector<compat_binary>
    to_binary(cluster::update_leadership_request obj) {
        return compat_binary::serde_and_adl(obj);
    }

    static void
    check(cluster::update_leadership_request obj, compat_binary test) {
        verify_adl_or_serde(obj, std::move(test));
    }
};
