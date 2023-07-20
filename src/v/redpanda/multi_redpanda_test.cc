#include "redpanda/application.h"
#include "seastarx.h"
#include "syschecks/syschecks.h"

#include <seastar/core/alien.hh>
#include <seastar/util/log.hh>

#include <iostream>
#include <thread>
#include <utility>

ss::logger myfix("asdf");

constexpr const char* tmpl = R"###(
cluster_size: {cluster_size}
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
      fmt::arg("cluster_size", cluster_size),
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

int main() {
    syschecks::initialize_intrinsics();

    node n0(0, "./foodata/n0", 2, 4_GiB);
    node n1(1, "./foodata/n1", 2, 4_GiB);
    node n2(2, "./foodata/n2", 2, 4_GiB);

    n0.start();
    n1.start();
    n2.start();

    // i dunno how to know when the reactors are ready
    std::this_thread::sleep_for(std::chrono::seconds(5));

    n0.run_on(0, []() noexcept { myfix.info("hello from fixture"); });
    n1.run_on(1, []() noexcept { myfix.info("hello from fixture"); });
    n2.run_on(1, []() noexcept { myfix.info("hello from fixture"); });

    n0.stop();
    n1.stop();
    n2.stop();
}
