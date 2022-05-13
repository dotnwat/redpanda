libjson = library('json',
  'json.cc',
  dependencies: [common, seastar, rapidjson],
  implicit_include_directories : false)

json = declare_dependency(
  dependencies: [common, seastar, rapidjson],
  link_with: libjson)
