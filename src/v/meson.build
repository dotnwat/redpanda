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

version_h = vcs_tag(
  input : 'version.h.in',
  output : 'version.h',
)

seastar_testing = dependency('seastar_testing')
json2code = find_program('seastar-json2code')
hdrhist = dependency('hdr_histogram')
rapidjson = dependency('rapidjson')
avro = dependency('avro')
base64 = dependency('base64')

roaring = dependency('roaring')
yaml = dependency('yaml-cpp')
boost = dependency('boost', modules : ['program_options', 'thread',
'filesystem', 'unit_test_framework'])
zstd = dependency('libzstd')
ctre = dependency('ctre')
snappy = dependency('snappy')
zlib = dependency('zlib')

cc = meson.get_compiler('cpp')
protobuf = dependency('protobuf', modules : ['protobuf::libprotobuf',
'protobuf::libprotoc'])

hwloc = dependency('hwloc')
numa = cc.find_library('numa', required : true)

add_project_arguments('-Wno-non-virtual-dtor', language: 'cpp')

# newer meson..
#inc_dep = declare_dependency(
#  include_directories: include_directories('.'))
#add_project_dependencies(inc_dep, language: 'cpp')

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
subdir('cluster')
subdir('cloud_storage')
subdir('archival')
subdir('v8_engine')
subdir('kafka')
subdir('coproc')
subdir('pandaproxy')
subdir('redpanda')
