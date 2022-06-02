libseastar_testing_main = library('seastar_testing_main',
  'seastar_testing_main.cc',
  dependencies: [seastar_testing, absl],
  implicit_include_directories: false)

seastar_testing_main = declare_dependency(
  dependencies: [seastar_testing, absl],
  link_with: libseastar_testing_main)
