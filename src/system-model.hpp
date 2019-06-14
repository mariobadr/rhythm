#ifndef RHYTHM_SYSTEM_MODEL_HPP
#define RHYTHM_SYSTEM_MODEL_HPP

#include <cstdint>
#include <map>
#include <set>

#include "architecture.hpp"
#include "common.hpp"

namespace rhythm {

/**
 * The status of a kernel thread.
 */
enum class thread_status {
  /**
   * Before a thread has been created by another thread.
   */
  unknown,
  /**
   * When a thread is waiting to run on a core.
   */
  runnable,
  /**
   * When a thread is running on a core.
   */
  running,
  /**
   * When a thread is waiting for another thread to wake it up.
   */
  blocked,
  /**
   * When a thread has no more events.
   */
  finished,
};

/**
 * A model of a kernel thread.
 */
struct kernel_thread {
  explicit kernel_thread(thread_t tid) : id(tid), status(thread_status::unknown)
  {
  }

  /**
   * The unique thread ID.
   */
  thread_t id;

  /**
   * The current status of this thread.
   */
  thread_status status;

  /**
   * Locks held by this thread, in order of acquire.
   */
  std::set<address_t> locks_held;

  std::map<thread_t, event_m> safety_net;
};

/**
 * A model of an operating system scheduler.
 */
struct sched_m {
  /**
   * The IDs of threads that are running on cores.
   */
  std::set<thread_t> running_threads;

  /**
   * The IDs of threads that can run but are not assigned to a core, in order of arrival.
   */
  std::deque<thread_t> runnable_threads;

  /**
   * The assignments of running threads to cores.
   */
  std::map<thread_t, std::size_t> mapping;

  /**
   * The IDs of cores that are idle.
   */
  std::deque<std::size_t> idle_cores;
};

/**
 * @return The CPI rate at which a thread will progress, based on the core it is running on.
 */
cpi_t get_cpi(arch_m const &arch, sched_m const &sched, thread_t thread_id);

/**
 * @return The frequency of the core on which a thread is running.
 */
freq_t get_freq(arch_m const &arch, sched_m const &sched, thread_t thread_id);

/**
 * Schedule threads to cores based on the current transitions.
 */
void schedule(sched_m &sched, std::map<thread_t, kernel_thread> &threads, transition_t const &t);

template <typename ostream>
ostream &operator<<(ostream &os, thread_status const &status)
{
  switch(status) {
  case thread_status::running:
    os << "running";
    break;
  case thread_status::runnable:
    os << "runnable";
    break;
  case thread_status::blocked:
    os << "blocked";
    break;
  case thread_status::finished:
    os << "finished";
    break;
  case thread_status::unknown:
  default:
    os << "unknown";
    break;
  }

  return os;
}

} // namespace rhythm

#endif //RHYTHM_SYSTEM_MODEL_HPP
