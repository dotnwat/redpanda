#json2code = find_program('seastar-json2code.py')

swags = []
foreach swag : [
  'config',
  'cluster_config',
  'raft',
  'security',
  'status',
  'features',
  'broker',
  'partition',
  'hbadger',
  'transaction',
  'debug']
s = custom_target(
  output : swag + '.json.h',
  input : swag + '.json',
  command : [json2code, '-f', '@INPUT@', '-o', '@OUTPUT@'],
)
swags += [s]
endforeach
