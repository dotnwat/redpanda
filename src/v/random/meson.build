librandom = library('random',
  'fast_prng.h',
  'generators.h',
  dependencies: [common, seastar, absl],
  implicit_include_directories : false)

random = declare_dependency(
  dependencies: [common, seastar, absl],
  link_with: librandom)

t = executable('random_test',
  'random_test.cc',
  dependencies: [random, boost])
test('test_random', t)
