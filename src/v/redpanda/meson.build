subdir('admin/api-doc')

executable('redpanda',
  'admin_server.cc',
  'request_auth.cc',
  'application.cc',
  'main.cc',
  admin_api_h,
  dependencies: [common, cluster, config])

#  include_directories: include_dirs,
#  dependencies : [seastar, xxhash, hdrhist, boost, hwloc, numa, config, hashing],
#  link_with : [raft, kafka, cloud_storage, pandaproxy_rest, coproc,
#    pandaproxy_schema_registry, finjector, syschecks, archival],
#  cpp_args : '-DXXH_PRIVATE_API')
