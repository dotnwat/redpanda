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
type join_group_request = (
  client: client,
  group_id: int,
  member_id: int,
  group_instance_id: int, // -1 for null
  protocol_type: int,
  protocols: seq[join_group_request_protocol]);

// struct join_group_request_protocol {
//     kafka::protocol_name name{};
//     bytes metadata{};
// };
type join_group_request_protocol = (
  protocol_name: int,
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

machine coordinator {
  var ms: seq[join_group_response_member];

  start state init {
    on join_group_request_event do (req: join_group_request) {
      print "received join group request";
      send req.client, join_group_response_event, (error_code = 0, generation_id = 0, protocol_name = 0,
      leader = 0, member_id = 0, members = ms);
    }
  }
}
