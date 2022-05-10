from ubuntu:impish

run apt-get update

run DEBIAN_FRONTEND=noninteractive apt-get install -qq \
    python3 python3-pip python3-setuptools python3-wheel ninja-build \
    libboost1.74-all-dev libprotobuf-dev libprotoc-dev libcrypto++-dev \
    pkg-config libfmt-dev liblz4-dev libgnutls28-dev libc-ares-dev \
    libyaml-cpp-dev ragel clang libabsl-dev libsnappy-dev libxxhash-dev \
    libzstd-dev git python3-jsonschema xfslibs-dev valgrind systemtap-sdt-dev \
    libsctp-dev ccache python3-jinja2

run pip3 install meson

run git clone https://github.com/redpanda-data/crc32c /src/crc32c
run cd /src/crc32c && cmake -GNinja -DCRC32C_BUILD_TESTS=OFF \
    -DCRC32C_BUILD_BENCHMARKS=OFF -DCRC32C_USE_GLOG=OFF . && ninja install

run git clone https://github.com/redpanda-data/CRoaring.git /src/roaring
run cd /src/roaring && cmake -GNinja -DENABLE_ROARING_TESTS=OFF \
    -DROARING_BUILD_STATIC=ON . && ninja install

run git clone https://github.com/redpanda-data/base64 /src/base64
run cd /src/base64 && make lib/libbase64.o
run cd /src/base64 && cp lib/libbase64.o /usr/local/lib
run cd /src/base64 && cp include/libbase64.h /usr/local/include

#run git clone https://github.com/redpanda-data/avro /src/avro
#run cd /src/avro/lang/c++ && cmake -GNinja -DCMAKE_SKIP_INSTALL_ALL_DEPENDENCY=true . && \
#    ninja avrocpp avrocpp_s avrogencpp && ninja install

workdir /src/redpanda
cmd ["bash", "-c", "meson builddir && cd builddir && meson compile"]
