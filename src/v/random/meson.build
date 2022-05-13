librandom = library('random',
  'fast_prng.h',
  'generators.h',
  dependencies: [common, seastar, absl],
  implicit_include_directories : false)

random = declare_dependency(
  dependencies: [common, seastar, absl],
  link_with: librandom)
