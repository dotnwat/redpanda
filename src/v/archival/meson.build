libarchival = library('archival',
  'archival_policy.cc',
  'service.cc',
  'ntp_archiver_service.cc',
  'probe.cc',
  'types.cc',
  'upload_controller.cc',
  implicit_include_directories: false,
  dependencies: [common, s3, storage, cluster])

archival = declare_dependency(
  dependencies: [common, s3, storage, cluster],
  link_with: libarchival)
