machine client
{
  var coord: coordinator;
  var protos: seq[join_group_request_protocol];

  start state init {
    entry {
      coord = new coordinator();
      send coord, join_group_request_event, (client = this, group_id = 0,
      member_id = 0, group_instance_id = 0, protocol_type = "a", protocols =
      protos);
      receive {
        case join_group_response_event: (resp: join_group_response) {
          print "got response";
          assert false, "asdf";
        }
      }
    }
  }
}

