#include "system-model.hpp"

#include "spdlog/spdlog.h"

#include <cassert>

namespace rhythm {

core_m const &get_core(arch_m const &arch, sched_m const &sched, thread_t thread_id)
{
  auto mapping_it = sched.mapping.find(thread_id);
  assert(mapping_it != sched.mapping.end());

  assert(mapping_it->second < arch.cores.size());
  core_m const &core = arch.cores[mapping_it->second];

  return core;
}

cpi_t get_cpi(arch_m const &arch, sched_m const &sched, thread_t thread_id)
{
  core_m const &core = get_core(arch, sched, thread_id);

  auto cpi_it = core.type.cpi_rates.find(thread_id);
  assert(cpi_it != core.type.cpi_rates.end());

  return cpi_it->second;
}

freq_t get_freq(arch_m const &arch, sched_m const &sched, thread_t thread_id)
{
  core_m const &core = get_core(arch, sched, thread_id);

  return core.frequency;
}

void use_next_core(sched_m &sched, thread_t thread_id)
{
  assert(!sched.idle_cores.empty());

  auto const core_id = sched.idle_cores.front();
  sched.idle_cores.pop_front();
  sched.mapping[thread_id] = core_id;

  sched.running_threads.insert(thread_id);
}

void free_core(sched_m &sched, thread_t thread_id)
{
  auto it = sched.mapping.find(thread_id);
  assert(it != sched.mapping.end());

  sched.idle_cores.push_back(it->second);
  sched.mapping.erase(it);
}

void wake_up(sched_m &sched, kernel_thread &thread)
{
  thread_t const thread_id = thread.id;

  assert(sched.running_threads.find(thread_id) == sched.running_threads.end());

  sched.runnable_threads.push_back(thread_id);
  thread.status = thread_status::runnable;
}

void sleep(sched_m &sched, kernel_thread &thread)
{
  thread_t const thread_id = thread.id;

  assert(sched.running_threads.find(thread_id) != sched.running_threads.end());

  sched.running_threads.erase(thread_id);
  thread.status = thread_status::blocked;

  assert(sched.running_threads.find(thread_id) == sched.running_threads.end());
}

void kill(sched_m &sched, kernel_thread &thread)
{
  thread_t const thread_id = thread.id;

  assert(thread.status == thread_status::finished);
  assert(sched.running_threads.find(thread_id) != sched.running_threads.end());

  sched.running_threads.erase(thread_id);
}

void schedule(sched_m &sched, std::map<thread_t, kernel_thread> &threads, transition_t const &t)
{
  for(auto const &thread_id : t.to_wake) {
    wake_up(sched, threads.at(thread_id));
  }

  for(auto const &thread_id : t.to_sleep) {
    sleep(sched, threads.at(thread_id));

    free_core(sched, thread_id);
  }

  for(auto const &thread_id : t.to_kill) {
    kill(sched, threads.at(thread_id));

    free_core(sched, thread_id);
  }

  while(!sched.idle_cores.empty() && !sched.runnable_threads.empty()) {
    thread_t const thread_id = sched.runnable_threads.front();
    assert(threads.find(thread_id) != threads.end());

    use_next_core(sched, thread_id);
    threads.at(thread_id).status = thread_status::running;
    sched.runnable_threads.pop_front();
  }
}
} // namespace rhythm