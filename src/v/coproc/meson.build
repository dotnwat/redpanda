supervisor_service_h = custom_target(
  output : 'supervisor.h',
  input : 'gen.json',
  command : [rpcgen, '--service_file', '@INPUT@', '--output_file', '@OUTPUT@'],
)

coproc = library('coproc', [
    'api.cc',
    'types.cc',
    'logger.cc',
    'script_context.cc',
    'script_context_frontend.cc',
    'script_context_backend.cc',
    'script_context_router.cc',
    'pacemaker.cc',
    'offset_storage_utils.cc',
    'wasm_event.cc',
    'event_listener.cc',
    'script_dispatcher.cc',
    'event_handler.cc',
    'partition.cc',
    'partition_manager.cc',
    'reconciliation_backend.cc',
    supervisor_service_h],
  dependencies : [seastar, xxhash, config, cluster, hdrhist, roaring, absl, kafka_client],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, rpc, model, storage, security],
  implicit_include_directories : false,
  include_directories: include_dirs)
