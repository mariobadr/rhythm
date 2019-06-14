#include "barrier.hpp"

namespace rhythm {

void add_barrier(sync_m &sm, address_t address, std::size_t count)
{
  //assert(sm.barriers.find(address) == sm.barriers.end());
  assert(count > 0);

  sm.barriers.emplace(address, count);

  assert(sm.barriers.find(address) != sm.barriers.end());
  assert(sm.barriers.find(address)->second.count == count);
}

transition_t barrier_wait(sync_m &sm, thread_t thread_id, std::uint64_t address)
{
  auto barrier_it = sm.barriers.find(address);
  assert(barrier_it != sm.barriers.end());

  barrier_m &barrier = barrier_it->second;
  barrier.waiters.push_back(thread_id);

  transition_t t{};
  if(barrier.count == barrier.waiters.size()) {
    // All threads have arrived at the barrier.

    for(auto const &waiter : barrier.waiters) {
      if(waiter != thread_id) {
        t.to_wake.push_back(waiter);
      }
    }

    barrier.waiters.clear();
  } else {
    // Still waiting for all threads to arrive at the barrier.
    t.to_sleep.push_back(thread_id);
  }

  return t;
}
}