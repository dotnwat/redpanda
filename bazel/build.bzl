# TODO Bazel prefers -iquote "path" style includes in many cases. However, our
# source tree uses bracket <path> style for dependencies. We need a way to
# bridge this gap until we decide to fully switch over to Bazel at which point
# this hack can be removed. Many deps lists in the tree will probably need to be
# updated to include abseil explicitly when this is removed.
def _inject_copt_includes(deps):
    copts = []
    copts.append("-Iexternal/abseil-cpp~")
    return copts

def redpanda_cc_library(
        name,
        srcs = [],
        hdrs = [],
        strip_include_prefix = None,
        visibility = None,
        include_prefix = None,
        deps = []):
    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        visibility = visibility,
        include_prefix = include_prefix,
        strip_include_prefix = strip_include_prefix,
        deps = deps,
        copts = _inject_copt_includes(deps),
    )

def redpanda_cc_test(
        name,
        timeout,
        srcs = [],
        deps = []):
    native.cc_test(
        name = name,
        timeout = timeout,
        srcs = srcs,
        deps = deps,
        copts = _inject_copt_includes(deps),
    )
