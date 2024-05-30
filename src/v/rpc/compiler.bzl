def rpc_gen(name, src, out):
  native.genrule(
    name = name,
    srcs = [src],
    outs = [out],
    cmd = "$(location //src/v/rpc:rpc_compiler) --service_file $(SRCS) --output_file $@",
    tools = ["//src/v/rpc:rpc_compiler"],
)
