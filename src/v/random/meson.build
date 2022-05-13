random = declare_dependency(
  dependencies: [seastar, absl])

t = executable('test_random',
  'random_test.cc',
  dependencies: [random, boost])

test('random tests', t)
