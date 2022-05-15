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
#zlib = dependency('zlib')
#zstd = dependency('libzstd')
#snappy = dependency('snappy')
boost = dependency('boost',
  modules: [
    'program_options',
    'thread',
    'filesystem',
    'unit_test_framework'])

# dependencies that are not satisified by system libraries, and will be built
# from source.
seastar = dependency('seastar', include_type: 'system')
seastar_testing = dependency('seastar_testing')
crc32c = dependency('crc32c')
rapidjson = dependency('rapidjson')
ctre = dependency('ctre')
hdr_histogram = dependency('hdr_histogram')
base64 = dependency('base64')

version_h = vcs_tag(
  input: 'version.h.in',
  output: 'version.h',
)

common = declare_dependency(
  include_directories: include_directories('.'))

subdir('bytes')
subdir('hashing')
subdir('finjector')
subdir('test_utils')
subdir('random')
subdir('json')
subdir('config')
subdir('model')
subdir('security')
subdir('compression')
subdir('utils')
subdir('syschecks')
subdir('net')
subdir('http')
subdir('s3')
subdir('storage')
subdir('rpc')
subdir('raft')
