def json2code(name, src, out):
    cc_out = out.removesuffix(".hh") + ".cc"
    out_abs = "$(location " + out + ")"
    native.genrule(
        name = name,
        srcs = [src],
        outs = [out, cc_out],
        cmd = "$(location @seastar//:seastar-json2code) --create-cc -f $< -o " + out_abs,
        tools = ["@seastar//:seastar-json2code"],
    )

def json2code_def(name, src, out, defn):
    src_abs = "$(location " + src + ")"
    out_abs = "$(location " + out + ")"
    cc_out = out.removesuffix(".hh") + ".cc"
    native.genrule(
        name = name,
        srcs = [src, defn],
        outs = [out, cc_out],
        cmd = "$(location @seastar//:seastar-json2code) --create-cc -f " + src_abs + " -o " + out_abs,
        tools = ["@seastar//:seastar-json2code"],
    )
