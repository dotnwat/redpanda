libjson = library('json',
  'json.cc',
  dependencies: [common, seastar, rapidjson],
  implicit_include_directories : false)

json = declare_dependency(
  dependencies: [common, seastar, rapidjson],
  link_with: libjson)

t = executable('json_test',
  'tests/json_serialization_test.cc',
  dependencies: [json, seastar_testing_main])
test('json_serialization_test', t)
