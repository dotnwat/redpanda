gnutls = dependency('gnutls')
xxhash = dependency('libxxhash')
absl = dependency('absl',
  modules: [
    'absl::flat_hash_map',
    'absl::btree',
    'absl::flat_hash_set',
    'absl::node_hash_set',
    'absl::node_hash_map',
    'absl::random_seed_sequences'])

seastar = dependency('seastar', include_type: 'system')
crc32c = dependency('crc32c')

common = declare_dependency(
  include_directories: include_directories('.'))

subdir('bytes')
subdir('hashing')
subdir('finjector')
