libarchival = library('archival',
  'archival_policy.cc',
  'ntp_archiver_service.cc',
  'probe.cc',
  'types.cc',
  'upload_controller.cc',
  'segment_reupload.cc',
  'retention_calculator.cc',
  implicit_include_directories: false,
  dependencies: [s3, storage])

archival = declare_dependency(
  dependencies: [s3, storage],
  link_with: libarchival)
