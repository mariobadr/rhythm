#include "statistics.hpp"

#include <fstream>

namespace rhythm {

void update_blocked_thread(sync_tracker &thread, time_t const elapsed)
{
  auto const &event = thread.last_event;

  switch(event.type) {
  case event_t::lock_acquire:
    thread.lock_wait_times[thread.last_event.object] += elapsed;
    break;
  case event_t::barrier_wait:
    thread.barrier_wait_times[thread.last_event.object] += elapsed;
    break;
  case event_t::condition_wait:
    thread.condition_wait_times[thread.last_event.object] += elapsed;
    break;
  default:
    break;
  }
}

void update(stats_t &stats, time_t elapsed, event_m const &event, sync_m const &sm)
{
  for(auto const &tid : sm.live_threads) {
    auto const &thread = sm.threads.at(tid);
    stats.run_time[tid] += elapsed;
    stats.status_time[tid].times[thread.status] += elapsed;

    if(thread.status == thread_status::blocked) {
      update_blocked_thread(stats.sync_time[tid], elapsed);
    }
  }

  stats.sync_time[event.thread_id].last_event = event;
}

void print_time_stacks(stats_t const &stats, std::string const &output_file)
{
  std::ofstream out(output_file);
  out << "TID,status,time\n";

  for(auto const &thread : stats.status_time) {
    for(auto const &pair : thread.second.times) {
      auto const status = pair.first;
      auto const time = std::chrono::duration<double>(pair.second);

      out << thread.first << "," << status << "," << time.count() << "\n";
    }
  }

  for(auto const &pair : stats.run_time) {
    auto const time = std::chrono::duration<double>(pair.second);

    out << pair.first << ",total," << time.count() << "\n";
  }
}

void print_sync_stacks(stats_t const &stats, std::string const &output_file)
{
  std::ofstream out(output_file);
  out << "TID,synchronization,address,time\n";

  for(auto const &thread_sync: stats.sync_time) {
    thread_t const thread_id = thread_sync.first;
    auto const &tracker = thread_sync.second;

    for(auto const &pair: tracker.lock_wait_times) {
      address_t const address = pair.first;
      auto const time = std::chrono::duration<double>(pair.second);

      out << thread_id << ",lock," << address << "," << time.count() << "\n";
    }

    for(auto const &pair: tracker.barrier_wait_times) {
      address_t const address = pair.first;
      auto const time = std::chrono::duration<double>(pair.second);

      out << thread_id << ",barrier-wait," << address << "," << time.count() << "\n";
    }

    for(auto const &pair: tracker.condition_wait_times) {
      address_t const address = pair.first;
      auto const time = std::chrono::duration<double>(pair.second);

      out << thread_id << ",condition-wait," << address << "," << time.count() << "\n";
    }
  }
}

void print(stats_t const &stats, std::string const &output_directory)
{
  print_time_stacks(stats, output_directory + "/rhythm-time-stacks.csv");
  print_sync_stacks(stats, output_directory + "/rhythm-sync-stacks.csv");
}

} // namespace rhythm