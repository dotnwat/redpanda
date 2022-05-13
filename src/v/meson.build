# dependencies that are satisifed by system libraries on supported
# distributions.
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
yaml = dependency('yaml-cpp')

# dependencies that are not satisified by system libraries, and will be built
# from source.
seastar = dependency('seastar', include_type: 'system')
seastar_testing = dependency('seastar_testing')
crc32c = dependency('crc32c')
rapidjson = dependency('rapidjson')
