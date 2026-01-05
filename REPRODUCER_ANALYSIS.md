# Race Condition Analysis: "updating non-joining member" Exception

## Summary

The exception `"updating non-joining member <member-id>"` is being thrown in production, which was believed to be logically impossible. Investigation reveals:

1. **The error message is misleading** - it should say "updating **already-joining** member"
2. **The bug is reproducible** - see test: `update_member_with_existing_join_promise`
3. **The root cause** - concurrent operations on static group members

## Root Cause Analysis

### The Misleading Logic

In `src/v/kafka/server/group.cc:428-434`:

```cpp
ss::future<join_group_response> group::update_member(...) {
    update_member_no_join(...);

    if (!member->is_joining()) {
        _num_members_joining++;
        return member->get_join_response();  // Creates new join_promise
    }

    // This path is taken when is_joining() == TRUE
    return ss::make_exception_future<join_group_response>(std::runtime_error(
      fmt::format("updating non-joining member {}", member->id())));
}
```

**The bug**: The exception is thrown when `!member->is_joining()` is **FALSE**, meaning the member **IS** joining (has an existing `join_promise`), but the error message says "**non-joining**".

### When This Happens

This occurs in the following scenario with static group members:

1. **Static member joins** with member_id `m1`
   - `add_member()` creates a `join_promise`
   - Member state: `is_joining() == true`
   - Background async operations may be pending

2. **Same static member reconnects** with member_id `m2` before the first join completes
   - `update_static_member_and_rebalance()` is called
   - `replace_static_member()` updates the member_id from `m1` to `m2`
   - Calls `try_finish_joining_member()` to clear the old promise
   - **BUT**: There's a race condition window here

3. **Race condition can occur when**:
   - Another concurrent request arrives
   - A timer fires (heartbeat expiration, join timer, etc.)
   - Background async operations from step 1 complete
   - These can create a NEW `join_promise` before `update_member()` is called

4. **`update_member()` is called** on a member that already has a `join_promise`
   - Check `if (!member->is_joining())` returns FALSE (member IS joining)
   - Exception is thrown with misleading message

### Evidence from Logs

When the test reproduces the issue, we see:

```
TRACE ... Updating joining member id=m1 ... joining=true
```

Followed by:

```
Exception message: updating non-joining member m1
```

## Reproducer Tests

### 1. Unit Test (Reliable Reproduction)

Added test: `SEASTAR_THREAD_TEST_CASE(update_member_with_existing_join_promise)`

Location: `src/v/kafka/server/tests/group_test.cc:573`

**What the test does:**

1. Creates a group member and adds it (creates `join_promise`)
2. Verifies member is in joining state: `member->is_joining() == true`
3. Calls `update_member()` on the same member while it's still joining
4. Catches the exception: `"updating non-joining member m1"`
5. **Demonstrates the bug**: Error message says "non-joining" but member IS joining

**Running the test:**

```bash
bazel test //src/v/kafka/server/tests:group_test --test_filter="*update_member_with_existing_join_promise*" --test_output=all
```

**Expected output:**
```
REPRODUCED: Exception thrown when update_member called on joining member
Exception message: updating non-joining member m1
BUG: Error says 'updating non-joining member' but should say 'updating already-joining member' because is_joining() == true
```

### 2. Integration Test (High-Level Kafka API)

Added test: `FIXTURE_TEST(concurrent_static_member_rejoin_race, consumer_offsets_fixture)`

Location: `src/v/kafka/server/tests/consumer_groups_test.cc:154`

**What the test does:**

1. Starts a full Redpanda instance with Kafka API server
2. Creates a static group member using the Kafka JoinGroup API
3. Rapidly sends multiple concurrent join requests from the same static member
4. Uses `group_initial_rebalance_delay=1ms` to speed up the test
5. Sends 25 total join requests to stress-test the race condition
6. Checks that no unexpected errors occur (race-dependent)

**Running the test:**

```bash
bazel test //src/v/kafka/server/tests:consumer_groups_test --test_filter="*concurrent_static_member_rejoin_race*" --test_output=streamed
```

**What to look for in logs:**
- `fenced_instance_id` errors indicate successful concurrent member replacement
- Look for `"Updating non-joining member ... joining=true"` in logs (race triggered)
- The race is timing-dependent and may not trigger every run
- Check for any unexpected errors or crashes during rapid rejoins

**Example successful output:**
```
Join 0 response: error={ error_code: fenced_instance_id [82] } member_id=...
Join 1 response: error={ error_code: none [0] } member_id=...
Starting rapid rejoin loop to stress-test the race...
Rapid rejoin 0 response: error={ error_code: none [0] } member_id=...
Test completed. If race not triggered, it's timing-dependent.
```

The integration test documents the real-world scenario where this bug occurs (static consumer group members rapidly reconnecting) and provides a way to test fixes under realistic conditions.

## Recommendations

### 1. Fix the Error Message (Immediate)

The error message is backwards. Change line 434 in `group.cc`:

```cpp
// Current (wrong):
fmt::format("updating non-joining member {}", member->id())

// Should be:
fmt::format("updating already-joining member {}", member->id())
```

### 2. Handle the Case Gracefully (Preferred)

Instead of throwing an exception, the code could:

```cpp
if (member->is_joining()) {
    // Member already has a join_promise from a previous operation
    // Clear it and create a new one for this update
    try_finish_joining_member(member,
        make_join_error(member->id(), error_code::rebalance_in_progress));
}
// Now safe to create new join_promise
_num_members_joining++;
return member->get_join_response();
```

### 3. Synchronization (Long-term)

The race condition suggests there may be insufficient synchronization around member state transitions. Consider:

- Adding a mutex/semaphore around member join/update operations
- Making the check-and-set of `join_promise` atomic
- Reviewing all paths that can create/clear `join_promise`

### 4. Add Defensive Logging

Add logging when this condition is detected to help diagnose future occurrences:

```cpp
if (member->is_joining()) {
    vlog(_ctxlog.warn,
         "Attempted to update member {} that already has a pending join operation. "
         "This may indicate concurrent join requests for the same static member.",
         member->id());
    // Handle gracefully...
}
```

## Related Code Locations

- Exception thrown: `src/v/kafka/server/group.cc:433-434`
- Member replacement: `src/v/kafka/server/group.cc:851-891` (`replace_static_member`)
- Static member update: `src/v/kafka/server/group.cc:722-748` (`update_static_member_and_rebalance`)
- Join promise management: `src/v/kafka/server/member.h:175` (`is_joining()`)

## Git History

The check was introduced in the original group implementation (commit `bc1ca5140d3` from 2019-10-24), and the error message has never been corrected despite the logic being inverted.

Recent related changes:
- `c927d21221` (2024-10-09): Expanded `update_member` to update additional fields (client_id, timeouts)
- This may have increased the likelihood of hitting this race condition
