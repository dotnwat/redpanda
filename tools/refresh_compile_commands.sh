#!/bin/bash

bazel run @hedron_compile_commands//:refresh_all -- \
  --features=-parse_headers \
  --host_features=-parse_headers \
  --config=system-clang-18
