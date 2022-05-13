

#    find_program(GENERATOR "seastar-json2code.py")
#    set (generator "${GENERATOR}")
#  endif()
#
#  add_custom_command (
#    DEPENDS
#      ${args_IN_FILE}
#      ${generator}
#    OUTPUT ${args_OUT_FILE}
#    COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
#    COMMAND ${generator} -f ${args_IN_FILE} -o ${args_OUT_FILE})

subdir('admin')

executable('redpanda', ['application.cc', 'admin_server.cc', raftgen_service_h,
  swags,
  'request_auth.cc',
  'main.cc'],
  include_directories: include_dirs,
  dependencies : [seastar, xxhash, hdrhist, boost, hwloc, numa, config, hashing],
  link_with : [raft, kafka, cloud_storage, pandaproxy_rest, coproc,
    pandaproxy_schema_registry, finjector, syschecks, archival],
  cpp_args : '-DXXH_PRIVATE_API')
