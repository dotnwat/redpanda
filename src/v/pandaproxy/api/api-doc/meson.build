#json2code = find_program('seastar-json2code.py')

pp_swags = []
foreach swag : [
  'rest', 'schema_registry']
s = custom_target(
  output : swag + '.json.h',
  input : swag + '.json',
  command : [json2code, '-f', '@INPUT@', '-o', '@OUTPUT@'],
)
pp_swags += [s]
endforeach
