#include "rhythm.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"

#include "architecture.hpp"
#include "controller.hpp"
#include "synchronization-model.hpp"
#include "system-model.hpp"
#include "trace.hpp"

namespace rhythm {

void create_master_thread(sched_m &sched, sync_m &sm, thread_t thread_id = DEFAULT_MASTER_THREAD_ID)
{
  sm.threads.at(thread_id).status = thread_status::running;

  sched.running_threads.insert(thread_id);
  sm.live_threads.insert(thread_id);

  sched.mapping.emplace(thread_id, 0);
  sched.idle_cores.pop_front();
}

void estimate(std::string const &manifest_file,
    std::string const &config_file,
    std::string const &output_dir)
{
  spdlog::get("log")->info("Loading model configuration file: {}", config_file);
  arch_m arch = parse_config_file(config_file);
  spdlog::get("log")->info("Model configuration file loaded successfully.", config_file);

  sync_m sm{};

  spdlog::get("log")->info("Loading trace manifest file: {}", manifest_file);
  app_m app = parse_traces(manifest_file, sm);
  spdlog::get("log")->info("All trace files loaded successfully.");

  spdlog::get("log")->info("{}", sm);
  spdlog::get("log")->info("{}", app);
  for(auto const &tm : app.threads) {
    spdlog::get("log")->info("{}", tm.second);
  }

  sched_m sched{};
  for(std::size_t core_id = 0; core_id < arch.cores.size(); ++core_id) {
    sched.idle_cores.push_back(core_id);
  }

  // Analogous to running the "main" function of a program.
  create_master_thread(sched, sm);
  pop_current_event(app.threads.at(DEFAULT_MASTER_THREAD_ID));

  stats_t stats{};

  spdlog::get("log")->info("Starting estimation.");

  while(!sm.live_threads.empty()) {
    auto const elapsed_time = step(app, arch, sched, sm, stats);
    stats.total_time += elapsed_time;
  }

  // Using a duration with type double gives us the time in seconds.
  auto const execution_time = std::chrono::duration<double>(stats.total_time).count();
  spdlog::get("log")->info("Done! Execution time is estimated to be {}s.", execution_time);

  print(stats, output_dir);
}

} // namespace rhythm