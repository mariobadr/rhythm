#include "lock.hpp"

#include "spdlog/spdlog.h"

namespace rhythm {

void add_lock(sync_m &sm, address_t address)
{
  assert(sm.locks.find(address) == sm.locks.end());

  sm.locks.emplace(address, lock_m{});

  assert(sm.locks.find(address) != sm.locks.end());
}

void grant_lock(sync_m &sm, thread_t thread_id, std::uint64_t address)
{
  sm.locks.at(address).held_by = thread_id;
  sm.threads.at(thread_id).locks_held.insert(address);
}

transition_t acquire(sync_m &sm, thread_t thread_id, std::uint64_t address)
{
  if(sm.locks.find(address) == sm.locks.end()) {
    spdlog::get("log")->warn("Encountered a lock that was not initialized.");
    add_lock(sm, address);
  }

  transition_t t{};
  lock_m &lock = sm.locks.at(address);

  if(lock.held_by == INVALID_THREAD_ID) {
    // No contention.
    grant_lock(sm, thread_id, address);
  } else {
    // Contention.
    lock.waiters.push_back(thread_id);
    t.to_sleep.push_back(thread_id);
  }

  return t;
}

transition_t release(sync_m &sm, thread_t thread_id, std::uint64_t address)
{
  auto lock_it = sm.locks.find(address);
  assert(lock_it != sm.locks.end());

  lock_m &lock = lock_it->second;
  assert(lock.held_by == thread_id);

  sm.threads.at(thread_id).locks_held.erase(address);

  transition_t t{};
  if(lock.waiters.empty()) {
    // No contention.
    lock.held_by = INVALID_THREAD_ID;
  } else {
    // Contention.
    thread_t next_thread_id = lock.waiters.front();
    lock.waiters.pop_front();
    grant_lock(sm, next_thread_id, address);

    t.to_wake.push_back(next_thread_id);
  }

  return t;
}

}