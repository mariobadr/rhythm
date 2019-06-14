#ifndef RHYTHM_SYNCHRONIZATION_MODEL_HPP
#define RHYTHM_SYNCHRONIZATION_MODEL_HPP

#include <cstdint>
#include <deque>
#include <map>
#include <set>

#include "common.hpp"
#include "system-model.hpp"

#include "spdlog/fmt/ostr.h"

namespace rhythm {

/**
 * A model for barrier synchronization.
 */
struct barrier_m {
  explicit barrier_m(std::size_t total_threads) : count(total_threads)
  {
  }

  /**
   * The number of threads that need to reach the barrier.
   */
  std::size_t count;

  /**
   * Threads waiting on this barrier, in order of arrival.
   */
  std::deque<thread_t> waiters;
};

/**
 * A model for condition variable synchronization.
 */
struct condition_variable_m {
  /**
   * The threads that signal on this condition variable.
   */
  std::set<thread_t> signallers;

  /**
   * The maximum number of signals.
   */
  std::uint64_t signal_count;

  /**
   * The threads that broadcast on this condition variable.
   */
  std::set<thread_t> broadcasters;

  /**
   * The maximum number of broadcasts.
   */
  std::uint64_t broadcast_count;

  /**
   * The last thread to broadcast.
   */
  thread_t last_broadcaster = INVALID_THREAD_ID;

  /**
   * The threads that wait on this condition variable.
   */
  std::set<thread_t> consumers;

  /**
   * An approximation of internal application state.
   */
  std::uint64_t production = 0;

  /**
   * Threads waiting on this condition variable, in order of arrival.
   */
  std::deque<thread_t> waiters;

  /**
   * The addresses of locks that need to be re-acquired by the threads in waiters.
   */
  std::deque<address_t> mutexes;
};

/**
 * A model for lock synchronization.
 */
struct lock_m {
  /**
   * Who is currently holding the lock.
   */
  thread_t held_by = INVALID_THREAD_ID;

  /**
   * Threads waiting for this lock, in order of arrival.
   */
  std::deque<thread_t> waiters;
};

/**
 * A synchronization model of, for example, a thread library.
 */
struct sync_m {
  /**
   * A model for each thread created.
   */
  std::map<thread_t, kernel_thread> threads;

  /**
   * Threads that have started and not finished.
   */
  std::set<thread_t> live_threads;

  /**
   * Threads that were created but have finished.
   */
  std::set<thread_t> finished_threads;

  /**
   * Threads that are blocked.
   */
  std::set<thread_t> blocked_threads;

  /**
   * A model of each barrier created.
   */
  std::map<address_t, barrier_m> barriers;

  /**
   * A model for each condition variable created.
   */
  std::map<address_t, condition_variable_m> condition_variables;

  /**
   * A model for each lock created.
   */
  std::map<address_t, lock_m> locks;

  /**
   * Threads that are waiting on others to finish.
   */
  std::map<thread_t, thread_t> join_queue;
};

/**
 * Add a barrier to the synchronization model.
 */
void add_barrier(sync_m &sm, address_t address, std::size_t count);

/**
 * Add a condition variable to the synchronization model.
 */
void add_condition_variable(sync_m &sm, address_t address);

/**
 * Update the condition variable model.
 */
void update_condition_variable(sync_m &sm, event_m event);

/**
 * Add a lock to the synchronization model.
 */
void add_lock(sync_m &sm, address_t address);

/**
 * Add a thread to the synchronization model.
 */
void add_thread(sync_m &sm, thread_t thread_id);

/**
 * Update internal state.
 *
 * Maintains the invariants of synchronization to ensure liveness and atomicity.
 *
 * @param event The synchronization event driving the update.
 *
 * @return The set of threads to be scheduled/slept.
 */
transition_t synchronize(sync_m &sm, event_m event);

/**
 * Break a deadlock that was caused due to approximating application state.
 */
transition_t break_deadlock(sync_m &sm, thread_t thread_id);

template <typename ostream>
ostream &operator<<(ostream &os, sync_m const &sm)
{
  os << "[synchronization_model] ";
  os << "Threads: " << sm.threads.size() << ", ";
  os << "Barriers: " << sm.barriers.size() << ", ";
  os << "Condition Variables: " << sm.condition_variables.size() << ", ";
  os << "Locks: " << sm.locks.size();

  return os;
}

} // namespace rhythm

#endif //RHYTHM_SYNCHRONIZATION_MODEL_HPP
