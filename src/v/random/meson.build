random = declare_dependency(
  dependencies: [common, seastar, absl])

t = executable('test_random',
  'random_test.cc',
  dependencies: [random, boost])

test('random tests', t)
