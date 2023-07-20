#include "redpanda/application.h"
#include "seastarx.h"
#include "syschecks/syschecks.h"

#include <seastar/core/alien.hh>
#include <seastar/util/log.hh>

#include <iostream>
#include <thread>

ss::logger myfix("asdf");

constexpr const char* tmpl = R"###(
cluster_size: 3
config_path: data/node{}/config.yaml
index: {}
redpanda:
  enable_pid_file: false
  admin:
    address: 0.0.0.0
    port: {}
  advertised_rpc_api:
    address: 0.0.0.0
    port: {}
  data_directory: data/node{}/data
  empty_seed_starts_cluster: false
  kafka_api:
    address: 0.0.0.0
    port: {}
  rack: null
  rpc_server:
    address: 0.0.0.0
    port: {}
  seed_servers:
  - address: 0.0.0.0
    port: 33145
  - address: 0.0.0.0
    port: 33146
  - address: 0.0.0.0
    port: 33147
)###";

namespace {
void make_config(int node_id) {
    std::ofstream out(fmt::format("data/node{}/config.yaml", node_id));
    auto config = fmt::format(
      tmpl,
      node_id,
      node_id,
      9644 + node_id,
      33145 + node_id,
      node_id,
      9092 + node_id,
      33145 + node_id);
    out << config;
    out.close();
}
} // namespace

int main() {
    syschecks::initialize_intrinsics();

    application app0("a");
    application app1("b");
    application app2("c");

    std::thread rp_0([&app = app0] {
        make_config(0);
        std::vector<std::string> args = {
          "testhingy",
          "--redpanda-cfg",
          fmt::format("data/node{}/config.yaml", 0),
          "-c",
          "2"};
        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }
        return app.run(argv.size(), argv.data());
    });

    std::thread rp_1([&app = app1] {
        make_config(1);
        std::vector<std::string> args = {
          "testhingy",
          "--redpanda-cfg",
          fmt::format("data/node{}/config.yaml", 1),
          "-c",
          "2"};
        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }
        return app.run(argv.size(), argv.data());
    });

    std::thread rp_2([&app = app2] {
        make_config(2);
        std::vector<std::string> args = {
          "testhingy",
          "--redpanda-cfg",
          fmt::format("data/node{}/config.yaml", 2),
          "-c",
          "2"};
        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }
        return app.run(argv.size(), argv.data());
    });

    // i dunno how to know when the reactors are ready
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ss::alien::run_on(
      app0.app.alien(), 0, []() noexcept { myfix.info("hello from fixture"); });
    ss::alien::run_on(
      app1.app.alien(), 1, []() noexcept { myfix.info("hello from fixture"); });
    ss::alien::run_on(
      app2.app.alien(), 1, []() noexcept { myfix.info("hello from fixture"); });

    rp_0.join();
    rp_1.join();
    rp_2.join();
}
