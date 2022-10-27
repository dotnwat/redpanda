/*
 * base commit: 8c771768b75cd4083a7063daa3f147d50b7a0532
 */

// struct join_group_request_data {
//     kafka::group_id group_id{};
//     kafka::member_id member_id{};
//     std::optional<kafka::group_instance_id> group_instance_id{};
//     kafka::protocol_type protocol_type{};
//     std::vector<join_group_request_protocol> protocols{};
// };
//
// version: annotation from client protocol negotation not in the actual request
type join_group_request = (
  client: client,
  version: int,
  group_id: int,
  member_id: int,
  group_instance_id: int, // -1 for null
  protocol_type: string,
  protocols: seq[join_group_request_protocol]);

// struct join_group_request_protocol {
//     kafka::protocol_name name{};
//     bytes metadata{};
// };
type join_group_request_protocol = (
  name: string,
  metadata: int);

//struct join_group_response_data {
//    kafka::error_code error_code{};
//    kafka::generation_id generation_id{-1};
//    kafka::protocol_name protocol_name{};
//    kafka::member_id leader{};
//    kafka::member_id member_id{};
//    std::vector<join_group_response_member> members{};
//};
type join_group_response = (
  error_code: int,
  generation_id: int,
  protocol_name: int,
  leader: int,
  member_id: int,
  members: seq[join_group_response_member]);

//struct join_group_response_member {
//    kafka::member_id member_id{};
//    std::optional<kafka::group_instance_id> group_instance_id{};
//    bytes metadata{};
//};
type join_group_response_member = (
  member_id: int,
  group_instance_id: int, // -1 for null
  metadata: int);

event join_group_request_event: join_group_request;
event join_group_response_event : join_group_response;

enum group_state { empty, preparing_rebalance, dead }
type group = (
  st: group_state,
  protocol_type: (isset:bool, value:string),
  supported_protocols: map[string, int],
  pending_members: map[int, int],
  members: map[int, int],
  initial_join_in_progress: bool);

fun make_group(): group {
  var sp: map[string, int];
  var m: map[int, int];
  var pm: map[int, int];
  return (
    st = empty,
    protocol_type = (isset = false, value = ""),
    supported_protocols = sp,
    pending_members = pm,
    members = m,
    initial_join_in_progress = false);
}

machine coordinator {
  var groups: map[int, group];
  var ms: seq[join_group_response_member];

  start state init {
    on join_group_request_event do (req: join_group_request) {
      var is_new_group: bool; // false

      if (!(req.group_id in groups)) {
        assert req.member_id == -1, "known member cannot join unknown group";
        groups += (req.group_id, make_group());
        is_new_group = true;
      }

      handle_join_group(req, is_new_group);

      print "received join group request";
      send req.client, join_group_response_event, (error_code = 0, generation_id = 0, protocol_name = 0,
      leader = 0, member_id = 0, members = ms);
    }
  }

  fun handle_join_group(req: join_group_request, is_new_group: bool) {
      var group: group;

      if (req.member_id == -1) { // unknown member
          join_group_unknown_member(req);
      } else {
          join_group_known_member(req);
      }

      // since group might have been updated, look for it again. this value
      // semantics stuff is nice, but it would be useful in this situation. i
      // wonder if there is another pattern that can be used.
      group = groups[req.group_id];
      if (!is_new_group && !group.initial_join_in_progress && group.st == preparing_rebalance) {
        // todo
      }
  }

  fun join_group_unknown_member(req: join_group_request) {
      if (in_state(req.group_id, dead)) {
      } else if (!supports_protocols(req)) {
      }

      //auto new_member_id = group::generate_member_id(r);
      //if (r.data.group_instance_id) {
      //    return add_new_static_member(std::move(new_member_id), std::move(r));
      //}

      add_new_dynamic_member(req, "new member id");
  }

  fun join_group_known_member(req: join_group_request) {
  }

  fun in_state(id: int, st: group_state): bool {
    assert id in groups;
    return groups[id].st == st;
  }

  fun add_new_dynamic_member(req: join_group_request, id: string) {
    if (req.version >= 4) {
      add_pending_member(req.group_id, 0);
    } else {
      //add_member_and_rebalance();
    }
  }

  fun add_pending_member(group_id: int, member_id: int) {
    if (member_id in groups[group_id].pending_members) {
      return;
    }

    groups[group_id].pending_members += (member_id, 0);
    // TODO: now we need to set a timer that when it fires it calls
    // remove_pending_member(member_id). we also want to be able to cancel the
    // timer when other execution paths remove entries from pending members.
  }

  // bool group::supports_protocols(const join_group_request& r) const;
  fun supports_protocols(req: join_group_request): bool {
    var group: group;
    var protocol: join_group_request_protocol;

    if (in_state(req.group_id, empty)) {
      return req.protocol_type != "" && sizeof(req.protocols) > 0;
    }

    group = groups[req.group_id];
    if (!group.protocol_type.isset ||
         group.protocol_type.value != req.protocol_type) {
      return false;
    }

    foreach(protocol in req.protocols) {
      if (protocol.name in group.supported_protocols) {
        if (group.supported_protocols[protocol.name] == sizeof(group.members)) {
          return true;
        }
      }
    }

    return false;
  }
}
