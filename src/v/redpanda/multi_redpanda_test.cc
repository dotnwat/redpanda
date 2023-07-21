#include "redpanda/application.h"
#include "seastarx.h"
#include "syschecks/syschecks.h"

#include <seastar/core/alien.hh>
#include <seastar/util/log.hh>

#include <kafka/AdminClient.h>
#include <kafka/KafkaProducer.h>

#include <iostream>
#include <thread>
#include <utility>

ss::logger myfix("asdf");

namespace multi_node_fixture {

constexpr const char* tmpl = R"###(
config_path: {config_file}
index: {node_id}
redpanda:
  enable_pid_file: false
  admin:
    address: 0.0.0.0
    port: {admin_port}
  advertised_rpc_api:
    address: 0.0.0.0
    port: {rpc_port}
  data_directory: {data_directory}
  empty_seed_starts_cluster: false
  kafka_api:
    address: 0.0.0.0
    port: {kafka_port}
  rack: null
  rpc_server:
    address: 0.0.0.0
    port: {rpc_port}
  seed_servers:
  - address: 0.0.0.0
    port: 33145
  - address: 0.0.0.0
    port: 33146
  - address: 0.0.0.0
    port: 33147
)###";

void write_config(
  int node_id,
  int cluster_size,
  std::filesystem::path config_file,
  std::filesystem::path data_directory) {
    std::ofstream out(config_file);
    auto config = fmt::format(
      tmpl,
      fmt::arg("node_id", node_id),
      fmt::arg("config_file", config_file),
      fmt::arg("data_directory", data_directory),
      fmt::arg("rpc_port", 33145 + node_id),
      fmt::arg("kafka_port", 9092 + node_id),
      fmt::arg("admin_port", 9644 + node_id));
    out << config;
    out.close();
}

class node {
    int id;
    int cores;
    size_t memory;
    std::filesystem::path dir;
    application app;
    std::thread thread;

public:
    node(int id, std::filesystem::path dir, int cores, size_t memory)
      : id(id)
      , cores(cores)
      , memory(memory)
      , dir(std::move(dir))
      , app(fmt::format("node-{}", id)) {}
    void start() {
        std::filesystem::create_directories(data_directory());
        write_config(id, 3, config_file(), data_directory());
        thread = std::thread([this] { return thread_func(); });
    }

    void stop() { thread.join(); }

    template<typename Func>
    void run_on(int core, Func func) {
        ss::alien::run_on(app.app.alien(), core, std::move(func));
    }

private:
    std::filesystem::path data_directory() const { return dir / "data"; }
    std::filesystem::path config_file() const { return dir / "config.yaml"; }

    std::vector<std::string> make_args() const {
        return {
          "multi-fixture",
          "--redpanda-cfg",
          config_file(),
          "--smp",
          fmt::format("{}", cores),
          "--memory",
          fmt::format("{}", memory),
        };
    }

    int thread_func() {
        auto args = make_args();

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }

        return app.run(static_cast<int>(argv.size()), argv.data());
    }
};

class cluster {
    std::filesystem::path dir;
    int cores;
    size_t memory;
    std::vector<std::unique_ptr<node>> nodes;

public:
    cluster(std::filesystem::path dir, int cores, size_t memory)
      : dir(dir)
      , cores(cores)
      , memory(memory) {}

    void add_node(int id) {
        auto node_dir = dir / fmt::format("node{}", id);
        nodes.push_back(std::make_unique<node>(id, node_dir, cores, memory));
        nodes.back()->start();
    }

    void stop() {
        for (auto& node : nodes) {
            node->stop();
        }
    }
};

} // namespace multi_node_fixture

int main() {
    syschecks::initialize_intrinsics();

    multi_node_fixture::cluster cluster("./foodata", 2, 2_GiB);
    cluster.add_node(0);
    cluster.add_node(1);
    cluster.add_node(2);

    try {
        const kafka::Topic topic("hello-topic");

        // Prepare the configuration
        kafka::Properties props;
        props.put("bootstrap.servers", "127.0.0.1:9092");
        kafka::clients::admin::AdminClient adminClient(props);
        adminClient.createTopics({topic}, 2, 3);

        // Create a producer
        kafka::clients::producer::KafkaProducer producer(props);

        std::string line = "This is going to be produced";
        kafka::clients::producer::ProducerRecord record(
          topic, kafka::NullKey, kafka::Value(line.c_str(), line.size()));

        for (int i = 0; i < 1000; i++) {
            producer.syncSend(record);
        }
        producer.close();

    } catch (...) {
        fmt::print("{}", std::current_exception());
    }

    cluster.stop();
}
