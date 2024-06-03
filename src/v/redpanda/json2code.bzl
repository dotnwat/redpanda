def json2code(name, src, out):
    native.genrule(
        name = name,
        srcs = [src],
        outs = [out],
        cmd = "$(location @seastar//:seastar-json2code) --create-cc -f $< -o $@",
        tools = ["@seastar//:seastar-json2code"],
    )
