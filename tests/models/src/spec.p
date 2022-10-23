spec GuaranteedWithDrawProgress observes join_group_request_event {
  start state NopendingRequests {
    on join_group_request_event do {
      print "spec got join group";
    }
  }
}
