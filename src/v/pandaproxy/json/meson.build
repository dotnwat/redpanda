libpandaproxy_json = library('pandaproxy_json',
  'error.cc',
  implicit_include_directories: false,
  dependencies: [common])

pandaproxy_json = declare_dependency(
  dependencies: [common],
  link_with: libpandaproxy_json)
