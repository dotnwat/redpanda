#!/bin/bash
set -e
set -x

export TZ="UTC"
export DEBIAN_FRONTEND=noninteractive
export PATH="${PATH}:/usr/local/go/bin"
export KAFKA_MIRROR="https://s3-us-west-2.amazonaws.com/kafka-packages"

function install_system_deps {
  apt update
  apt install -y \
        bind9-utils \
        bind9-dnsutils \
        bsdmainutils \
        build-essential \
        curl \
        default-jdk \
        dmidecode \
        cmake \
        git \
        iproute2 \
        iptables \
        libatomic1 \
        libyajl-dev \
        libsasl2-dev \
        libssl-dev \
        maven \
        pciutils \
        nodejs \
        npm \
        openssh-server \
        python3-pip
}

function update_limits {
  echo '* soft nofile 524288' >> /etc/security/limits.conf
  echo '* hard nofile 524288' >> /etc/security/limits.conf
  echo 'ubuntu soft nofile 524288' >> /etc/security/limits.conf
  echo 'ubuntu hard nofile 524288' >> /etc/security/limits.conf
  echo 'root soft nofile 524288' >> /etc/security/limits.conf
  echo 'root hard nofile 524288' >> /etc/security/limits.conf
}

function install_golang {
  mkdir -p /usr/local/go/
  curl -sSLf --retry 3 --retry-connrefused --retry-delay 2 \
      https://golang.org/dl/go1.17.linux-amd64.tar.gz | tar -xz -C /usr/local/go/ --strip 1
}

function install_kafka_tools {
  mkdir -p "/opt/kafka-2.3.1" && chmod a+rw /opt/kafka-2.3.1 && curl -s "$KAFKA_MIRROR/kafka_2.12-2.3.1.tgz" | tar xz --strip-components=1 -C "/opt/kafka-2.3.1"
  mkdir -p "/opt/kafka-2.4.1" && chmod a+rw /opt/kafka-2.4.1 && curl -s "$KAFKA_MIRROR/kafka_2.12-2.4.1.tgz" | tar xz --strip-components=1 -C "/opt/kafka-2.4.1"
  mkdir -p "/opt/kafka-2.5.0" && chmod a+rw /opt/kafka-2.5.0 && curl -s "$KAFKA_MIRROR/kafka_2.12-2.5.0.tgz" | tar xz --strip-components=1 -C "/opt/kafka-2.5.0"
  mkdir -p "/opt/kafka-2.7.0" && chmod a+rw /opt/kafka-2.7.0 && curl -s "$KAFKA_MIRROR/kafka_2.12-2.7.0.tgz" | tar xz --strip-components=1 -C "/opt/kafka-2.7.0"
}

function install_librdkafka {
  mkdir /opt/librdkafka
  curl -SL "https://github.com/edenhill/librdkafka/archive/v1.8.0.tar.gz" | tar -xz --strip-components=1 -C /opt/librdkafka
  cd /opt/librdkafka
  ./configure
  make -j$(nproc)
  make install
  cd /opt/librdkafka/tests
  make build -j$(nproc)
}

function install_kaf {
  go install github.com/birdayz/kaf/cmd/kaf@master
  mv /root/go/bin/kaf /usr/local/bin/
}

function install_kcat {
  mkdir /tmp/kcat
  curl -SL "https://github.com/edenhill/kcat/archive/1.7.0.tar.gz" | tar -xz --strip-components=1 -C /tmp/kcat
  cd /tmp/kcat
  ./configure
  make -j$(nproc)
  make install
  ldconfig
}

function install_sarama_examples {
  git -C /opt clone https://github.com/Shopify/sarama.git
  cd /opt/sarama/examples/interceptors && go mod tidy && go build
  cd /opt/sarama/examples/http_server && go mod tidy && go build
  cd /opt/sarama/examples/consumergroup && go mod tidy && go build
  cd /opt/sarama/examples/sasl_scram_client && go mod tidy && go build
}

function install_franz_bench {
  git -C /opt clone https://github.com/twmb/franz-go.git && cd /opt/franz-go
  cd /opt/franz-go/examples/bench && go mod tidy && go build
}

function install_kcl {
  go install github.com/twmb/kcl@latest
  mv /root/go/bin/kcl /usr/local/bin/
}

function install_kafka_streams_examples {
  git -C /opt clone --branch ducktape2 https://github.com/vectorizedio/kafka-streams-examples.git
  cd /opt/kafka-streams-examples && mvn -DskipTests=true clean package
}

function install_python_deps {
  python3 -m pip install \
    ducktape@git+https://github.com/vectorizedio/ducktape.git@6e2af9173a79feb8661c4c7a5776080721710a43 \
    prometheus-client==0.9.0 \
    pyyaml==5.3.1 \
    kafka-python==2.0.2 \
    crc32c==2.2 \
    confluent-kafka==1.7.0 \
    zstandard==0.15.2 \
    xxhash==2.0.2
}

install_system_deps
update_limits
install_golang
install_kafka_tools
install_librdkafka
install_kaf
install_kcat
install_sarama_examples
install_franz_bench
install_kcl
install_kafka_streams_examples
install_python_deps
