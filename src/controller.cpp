#include "controller.hpp"

#include <sstream>

#include "spdlog/spdlog.h"

#include "statistics.hpp"

namespace rhythm {

thread_t select_next_thread(app_m const &app,
    arch_m const &arch,
    sched_m const &sched,
    time_t &shortest_time)
{
  assert(!sched.running_threads.empty());

  shortest_time = time_t::max();
  thread_t next_thread = *sched.running_threads.begin();

  for(thread_t const &thread_id : sched.running_threads) {
    auto thread_it = app.threads.find(thread_id);
    assert(thread_it != app.threads.end());

    event_m const event = get_current_event(thread_it->second);
    icount_t const instructions = event.distance;

    cpi_t const cpi_rate = get_cpi(arch, sched, thread_id);
    freq_t const frequency = get_freq(arch, sched, thread_id);

    time_t time_till_event = estimate_time(instructions, cpi_rate, frequency);

    if(time_till_event < shortest_time) {
      next_thread = thread_id;
      shortest_time = time_till_event;
    }
  }

  return next_thread;
}

time_t step(app_m &app, arch_m &arch, sched_m &sched, sync_m &sm, stats_t &stats)
{
  time_t elapsed_time(0);

  // Select the next thread based on which thread will reach a synchronization event first.
  thread_t const current_thread = select_next_thread(app, arch, sched, elapsed_time);

  for(thread_t const &thread_id : sched.running_threads) {
    auto thread_it = app.threads.find(thread_id);
    assert(thread_it != app.threads.end());

    if(elapsed_time.count() > 0) {
      cpi_t const cpi_rate = get_cpi(arch, sched, thread_id);
      freq_t const frequency = get_freq(arch, sched, thread_id);
      icount_t const instructions = estimate_instructions(elapsed_time, cpi_rate, frequency);

      execute(thread_it->second, instructions);
    }
  }

  event_m const current_event = get_current_event(app.threads.at(current_thread));
  update(stats, elapsed_time, current_event, sm);

#ifndef NDEBUG
  std::ostringstream stream;
  for(auto const &t : sched.running_threads) {
    stream << t << ", ";
  }

  auto const running_time = stats.status_time[current_thread].times[thread_status::running].count();
  spdlog::get("rhythm-trace")
      ->info("{} [{} ns] [{} ns] [{} ns] [{}]", current_event, elapsed_time.count(), running_time,
          stats.total_time.count(), stream.str());
#endif

  transition_t const state_changes = synchronize(sm, current_event);
  schedule(sched, sm.threads, state_changes);

  if(sched.running_threads.empty() && !sm.live_threads.empty()) {
    spdlog::get("log")->info("Breaking deadlock.");
    transition_t const t = break_deadlock(sm, current_thread);
    schedule(sched, sm.threads, t);
  }

  assert(current_event.distance <= 1);
  pop_current_event(app.threads.at(current_thread));

  return elapsed_time;
}
} // namespace rhythm