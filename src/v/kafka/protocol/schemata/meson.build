schemas = [
  'api_versions',
  'offset_fetch',
  'offset_commit',
  'join_group',
  'sync_group',
  'heartbeat',
  'leave_group',
  'delete_topics',
  'describe_configs',
  'find_coordinator',
  'list_offset',
  'alter_configs',
  'list_groups',
  'describe_groups',
  'create_topics',
  'sasl_handshake',
  'sasl_authenticate',
  'init_producer_id',
  'incremental_alter_configs',
  'delete_groups',
  'describe_acls',
  'describe_log_dirs',
  'create_acls',
  'delete_acls',
  'produce',
  'metadata',
  'add_partitions_to_txn',
  'txn_offset_commit',
  'fetch',
  'end_txn',
  'create_partitions',
  'add_offsets_to_txn',
  'offset_for_leader_epoch']

codegen = find_program('generator.py')

hdrs = []
srcs = []
foreach schema : schemas
  foreach flavor : ['request', 'response']
    name = f'@schema@_@flavor@'
    t = custom_target(
      input: f'@name@.json',
      output: [f'@name@.h', f'@name@.cc'],
      command : [codegen, '@INPUT@', '@OUTPUT0@', '@OUTPUT1@'])
    hdrs += [t[0]]
    srcs += [t[1]]
  endforeach
endforeach
