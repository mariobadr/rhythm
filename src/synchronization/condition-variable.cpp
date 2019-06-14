#include "synchronization-model.hpp"

#include "spdlog/spdlog.h"

#include "condition-variable.hpp"
#include "lock.hpp"

namespace rhythm {

std::set<thread_t> find_subset(std::set<thread_t> const &set1, std::set<thread_t> const &set2)
{
  std::set<thread_t> subset;

  std::set_intersection(
      set1.begin(), set1.end(), set2.begin(), set2.end(), std::inserter(subset, subset.begin()));

  return subset;
}

void add_condition_variable(sync_m &sm, address_t address)
{
  assert(sm.condition_variables.find(address) == sm.condition_variables.end());

  sm.condition_variables.emplace(address, condition_variable_m{});

  assert(sm.condition_variables.find(address) != sm.condition_variables.end());
}

void update_safety_net(kernel_thread &thread, event_m event, std::set<thread_t> const &consumers)
{
  for(auto const &consumer : consumers) {
    event_m silent_event;

    silent_event.thread_id = thread.id;
    silent_event.type = event.type;
    silent_event.object = event.object;

    thread.safety_net.emplace(consumer, silent_event);
  }
}

void update_condition_variable(sync_m &sm, event_m event)
{
  if(sm.condition_variables.find(event.object) == sm.condition_variables.end()) {
    spdlog::get("log")->warn("Encountered a condition variable that was not initialized.");
    sm.condition_variables.emplace(event.object, condition_variable_m{});
  }

  if(event.type == event_t::condition_wait) {
    sm.condition_variables.at(event.object).consumers.insert(event.thread_id);
  } else if(event.type == event_t::condition_signal) {
    auto &cv = sm.condition_variables.at(event.object);

    cv.signallers.insert(event.thread_id);
    cv.signal_count++;

    update_safety_net(sm.threads.at(event.thread_id), event, cv.consumers);
  } else if(event.type == event_t::condition_broadcast) {
    auto &cv = sm.condition_variables.at(event.object);

    cv.broadcasters.insert(event.thread_id);
    cv.broadcast_count++;

    update_safety_net(sm.threads.at(event.thread_id), event, cv.consumers);
  } else {
    throw std::runtime_error("Unknown condition variable event type.");
  }

  assert(sm.condition_variables.find(event.object) != sm.condition_variables.end());
}

transition_t condition_broadcast(sync_m &sm, thread_t thread_id, std::uint64_t address)
{
  transition_t t{};

  auto cv_it = sm.condition_variables.find(address);
  assert(cv_it != sm.condition_variables.end());
  condition_variable_m &cv = cv_it->second;

  if(cv.consumers.empty()) {
    return t;
  }

  assert(cv.broadcast_count >= 1);
  cv.broadcast_count--;
  cv.last_broadcaster = thread_id;

  auto const live_consumers = find_subset(sm.live_threads, cv.consumers);
  assert(live_consumers.size() >= cv.waiters.size());

  auto production_estimate = live_consumers.size() - cv.waiters.size();
  cv.production = std::min(cv.consumers.size(), cv.production + production_estimate);

  if(!cv.waiters.empty()) {
    auto const priority_thread = cv.waiters.front();
    address_t const mutex = cv.mutexes.front();

    cv.waiters.pop_front();
    cv.mutexes.pop_front();

    transition_t check_acquire = acquire(sm, priority_thread, mutex);
    auto it =
        std::find(check_acquire.to_sleep.begin(), check_acquire.to_sleep.end(), priority_thread);

    if(it == check_acquire.to_sleep.end()) {
      // The lock acquire was successful.
      t.to_wake.push_back(priority_thread);
    }

    // The waiters are no longer waiting on a broadcast but rather a lock acquire.
    assert(cv.waiters.size() == cv.mutexes.size());
    for(std::size_t i = 0; i < cv.waiters.size(); i++) {
      acquire(sm, cv.waiters[i], cv.mutexes[i]);
    }

    cv.waiters.clear();
    cv.mutexes.clear();
  }

  return t;
}

transition_t condition_signal(sync_m &sm, std::uint64_t address)
{
  transition_t t{};

  auto cv_it = sm.condition_variables.find(address);
  assert(cv_it != sm.condition_variables.end());
  condition_variable_m &cv = cv_it->second;

  if(cv.consumers.empty()) {
    return t;
  }

  assert(cv.signal_count >= 1);
  cv.signal_count--;

  if(cv.waiters.empty()) {
    // No thread is waiting currently, but may in the future.
    cv.production = std::min(cv.consumers.size(), cv.production + 1);
  } else {
    thread_t const waiting_thread = cv.waiters.front();
    address_t const mutex = cv.mutexes.front();

    cv.waiters.pop_front();
    cv.mutexes.pop_front();

    transition_t const check_acquire = acquire(sm, waiting_thread, mutex);
    auto it =
        std::find(check_acquire.to_sleep.begin(), check_acquire.to_sleep.end(), waiting_thread);

    if(it == check_acquire.to_sleep.end()) {
      // The lock acquire was successful.
      t.to_wake.push_back(waiting_thread);
    }
  }

  if(cv.signal_count == 0) {
    assert(cv.waiters.size() == cv.mutexes.size());
    for(std::size_t i = 0; i < cv.waiters.size(); i++) {
      acquire(sm, cv.waiters[i], cv.mutexes[i]);
    }

    cv.waiters.clear();
    cv.mutexes.clear();
  }

  return t;
}

bool can_wait(sync_m const &sm, condition_variable_m const &cv, thread_t thread_id)
{
  auto const live_broadcasters = find_subset(sm.live_threads, cv.broadcasters);
  auto const live_signallers = find_subset(sm.live_threads, cv.signallers);

  if(live_broadcasters.empty() && live_signallers.empty()) {
    // None of the live threads can produce anything, so don't wait.
    return false;
  }

  if(live_broadcasters.size() == 1 &&
      live_broadcasters.find(thread_id) != live_broadcasters.end()) {
    // The only live broadcaster is the thread that called wait.
    return false;
  }

  if(live_signallers.size() == 1 && live_signallers.find(thread_id) != live_signallers.end()) {
    // The only live signaller is the thread that called wait.
    return false;
  }

  if(cv.broadcast_count == 0 && cv.signal_count == 0) {
    // There are no more available signals or broadcasts.
    return false;
  }

  return true;
}

transition_t
condition_wait(sync_m &sm, thread_t thread_id, std::uint64_t address, std::uint64_t mutex)
{
  transition_t t{};

  auto cv_it = sm.condition_variables.find(address);
  assert(cv_it != sm.condition_variables.end());
  condition_variable_m &cv = cv_it->second;

  if(cv.production > 0) {
    // There is no need to wait.
    cv.production--; // Consume production.

    return t;
  }

  if(!can_wait(sm, cv, thread_id)) {
    // Don't wait to maintain liveness.
    return t;
  }

  // Not enough production, we need to wait.
  cv.waiters.push_back(thread_id);
  cv.mutexes.push_back(mutex);

  t = release(sm, thread_id, mutex);
  t.to_sleep.push_back(thread_id);

  return t;
}

} // namespace rhythm