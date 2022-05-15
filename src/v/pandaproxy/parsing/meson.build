libpandaproxy_parsing = library('pandaproxy_parsing',
  'error.cc',
  implicit_include_directories: false,
  dependencies: [common])

pandaproxy_parsing = declare_dependency(
  dependencies: [common],
  link_with: libpandaproxy_parsing)
