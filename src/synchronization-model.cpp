#include "synchronization-model.hpp"

#include "spdlog/spdlog.h"

#include <cassert>

#include "synchronization/barrier.hpp"
#include "synchronization/condition-variable.hpp"
#include "synchronization/lock.hpp"

namespace rhythm {

void add_thread(sync_m &sm, thread_t thread_id)
{
  assert(sm.threads.find(thread_id) == sm.threads.end());

  sm.threads.emplace(thread_id, thread_id);

  assert(sm.threads.find(thread_id) != sm.threads.end());
}

transition_t create(sync_m &sm, thread_t thread_id)
{
  sm.threads.at(thread_id).status = thread_status::runnable;

  transition_t t{};
  t.to_wake.push_back(thread_id);

  return t;
}

transition_t start(sync_m &sm, thread_t thread_id)
{
  sm.live_threads.insert(thread_id);

  return transition_t{};
}

transition_t join(sync_m &sm, thread_t current_thread, thread_t target_thread)
{
  transition_t t{};

  auto target_it = sm.finished_threads.find(target_thread);
  if(target_it == sm.finished_threads.end()) {
    // The target thread has not finished yet, so the current_thread should wait.
    t.to_sleep.push_back(current_thread);

    // Create a dependency so that current_thread will wake up when target_thread finishes.
    sm.join_queue.emplace(target_thread, current_thread);
  } else {
    // The target thread has already finished, there is no need for the current thread to wait.
  }

  return t;
}

transition_t finish(sync_m &sm, thread_t thread_id)
{
  assert(sm.finished_threads.find(thread_id) == sm.finished_threads.end());

  transition_t t{};

  // Check if another thread was waiting on this one to finish.
  auto const it = sm.join_queue.find(thread_id);
  if(it != sm.join_queue.end()) {
    // Wake up the waiting thread.
    t.to_wake.push_back(it->second);

    // The dependency has been resolved.
    sm.join_queue.erase(it);
  }

  auto &thread = sm.threads.at(thread_id);

  for(auto const &held_lock : thread.locks_held) {
    spdlog::get("log")->warn("Thread {} finished while holding a lock ({}).", thread_id, held_lock);

    release(sm, thread_id, held_lock);
  }

  sm.finished_threads.insert(thread_id);
  thread.status = thread_status::finished;

  t.to_kill.push_back(thread_id);
  sm.live_threads.erase(thread_id);

  return t;
}

transition_t synchronize(sync_m &sm, event_m event)
{
  assert(event.type != event_t::unknown);

  transition_t t{};

  switch(event.type) {
  case event_t::thread_create:
    t = create(sm, event.target_thread);
    break;
  case event_t::thread_start:
    t = start(sm, event.thread_id);
    break;
  case event_t::thread_join:
    t = join(sm, event.thread_id, event.target_thread);
    break;
  case event_t::thread_finish:
    t = finish(sm, event.thread_id);
    break;
  case event_t::lock_acquire:
    t = acquire(sm, event.thread_id, event.object);
    break;
  case event_t::lock_release:
    t = release(sm, event.thread_id, event.object);
    break;
  case event_t::barrier_wait:
    t = barrier_wait(sm, event.thread_id, event.object);
    break;
  case event_t::condition_broadcast:
    t = condition_broadcast(sm, event.thread_id, event.object);
    break;
  case event_t::condition_signal:
    t = condition_signal(sm, event.object);
    break;
  case event_t::condition_wait:
    t = condition_wait(sm, event.thread_id, event.object, event.object2);
    break;
  default:
    spdlog::get("log")->warn("Unknown synchronization event.");
    break;
  }

  for(auto const &thread : t.to_sleep) {
    sm.blocked_threads.insert(thread);
  }

  for(auto const &thread : t.to_wake) {
    sm.blocked_threads.erase(thread);
  }

  for(auto const &thread : t.to_kill) {
    sm.blocked_threads.erase(thread);
  }

  return t;
}

transition_t break_deadlock(sync_m &sm, thread_t thread_id)
{
  // All threads are blocked, find the possible threads that we can wake up.
  auto &thread = sm.threads.at(thread_id);
  if(thread.safety_net.empty()) {
    throw std::runtime_error("All threads are blocked and there are no options.");
  }

  auto possibility_it = thread.safety_net.begin();
  for(; possibility_it != thread.safety_net.end(); possibility_it++) {
    if(sm.live_threads.find(possibility_it->first) != sm.live_threads.end()) {
      break;
    }
  }

  if(possibility_it == thread.safety_net.end()) {
    throw std::runtime_error("All threads are blocked and there are no live options.");
  }

  return synchronize(sm, possibility_it->second);
}

} // namespace rhythm