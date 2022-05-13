include_dirs = include_directories('.')

version_h = vcs_tag(
  input : 'version.h.in',
  output : 'version.h',
)

seastar = dependency('seastar')
seastar_testing = dependency('seastar_testing')
json2code = find_program('seastar-json2code')
hdrhist = dependency('hdr_histogram')
rapidjson = dependency('rapidjson')
avro = dependency('avro')
base64 = dependency('base64')
crc32c = dependency('crc32c')

xxhash = dependency('libxxhash')
roaring = dependency('roaring')
absl = dependency('absl', modules: ['absl::flat_hash_map', 'absl::btree',
'absl::flat_hash_set', 'absl::node_hash_set', 'absl::node_hash_map',
'absl::random_seed_sequences'])
yaml = dependency('yaml-cpp')
boost = dependency('boost', modules : ['program_options', 'thread',
'filesystem', 'unit_test_framework'])
zstd = dependency('libzstd')
gnutls = dependency('gnutls')
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
subdir('finjector')
subdir('random')
subdir('json')
subdir('config')
subdir('hashing')
subdir('bytes')
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
