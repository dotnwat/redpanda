librandom = library('random',
  'fast_prng.h',
  'generators.h',
  dependencies: [common, seastar, absl],
  implicit_include_directories : false)

random = declare_dependency(
  dependencies: [common, seastar, absl],
  link_with: librandom)

t = executable('test_random',
  'random_test.cc',
  dependencies: [random, boost])

test('random tests', t)
