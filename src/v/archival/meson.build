archival = library('archival', [
    'archival_policy.cc',
    'service.cc',
    'ntp_archiver_service.cc',
    'probe.cc',
    'types.cc',
    'upload_controller.cc'],
  dependencies : [seastar, absl, ctre, hdrhist, config, cluster],
  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
  link_with : [bytes, s3, cloud_storage, http, storage],
  implicit_include_directories : false,
  include_directories: include_dirs)
