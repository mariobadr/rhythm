#ifndef RHYTHM_COMMON_HPP
#define RHYTHM_COMMON_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <set>

namespace rhythm {

/**
 * Represent time at the granularity of nanoseconds.
 */
using time_t = std::chrono::nanoseconds;

/**
 * Represent thread IDs as unique integers.
 */
using thread_t = std::int64_t;

/**
 * An invalid thread ID.
 */
constexpr thread_t INVALID_THREAD_ID = -1;

/**
 * The thread ID to use for the initial master thread.
 */
constexpr thread_t DEFAULT_MASTER_THREAD_ID = 0;

/**
 * Represents a memory address.
 */
using address_t = std::uint64_t;

/**
 * Represent dynamic instruction counts as integers.
 */
using icount_t = std::uint64_t;

/**
 * Represent the rate of cycles-per-instruction (CPI) as a fractional value.
 */
using cpi_t = double;

/**
 * A frequency type in units of Hertz.
 */
using freq_t = std::int64_t;

/**
 * Transitions (e.g., context switches) that need to take place due to a synchronization event.
 */
struct transition_t {
  std::deque<thread_t> to_sleep;
  std::deque<thread_t> to_wake;
  std::deque<thread_t> to_kill;
};

/**
 * The type of synchronization event.
 */
enum class event_t {
  barrier_wait,
  condition_broadcast,
  condition_signal,
  condition_wait,
  lock_acquire,
  lock_release,
  thread_create,
  thread_finish,
  thread_join,
  thread_start,
  unknown
};

/**
 * A model of a synchronization event.
 */
struct event_m {
  /**
   * The thread that this event belongs to.
   */
  thread_t thread_id = -1;

  /**
   * The type (e.g., synchronization call) of the event.
   */
  event_t type = event_t::unknown;

  /**
   * The number of dynamic instructions before the event.
   */
  icount_t distance = 0;

  /**
   * The address of the synchronization object acted on.
   *
   * Only valid for certain event types.
   */
  std::uint64_t object = 0;

  /**
   * The address of a second synchronization object.
   *
   * Only valid for wait events on condition variables, which also require a mutex.
   */
  std::uint64_t object2 = 0;

  /**
   * The thread to wait for before continuing.
   *
   * Only valid for certain event types.
   */
  thread_t target_thread = -1;
};

template <typename ostream>
ostream &operator<<(ostream &os, event_t const &type)
{
  switch(type) {
  case event_t::lock_acquire:
    os << "acquire";
    break;
  case event_t::lock_release:
    os << "release";
    break;
  case event_t::condition_wait:
    os << "condition_wait";
    break;
  case event_t::condition_signal:
    os << "signal";
    break;
  case event_t::condition_broadcast:
    os << "broadcast";
    break;
  case event_t::barrier_wait:
    os << "barrier_wait";
    break;
  case event_t::thread_start:
    os << "start";
    break;
  case event_t::thread_join:
    os << "join";
    break;
  case event_t::thread_create:
    os << "create";
    break;
  case event_t::thread_finish:
    os << "finish";
    break;
  default:
    os << "unknown";
    break;
  }
  return os;
}

template <typename ostream>
ostream &operator<<(ostream &os, event_m const &em)
{
  os << "[event_model] ";
  os << "[TID: " << em.thread_id << "] ";
  os << "[" << em.type << "] ";

  switch(em.type) {
  case event_t::lock_acquire:
  case event_t::lock_release:
  case event_t::barrier_wait:
  case event_t::condition_signal:
  case event_t::condition_broadcast:
    os << "[" << em.object << "]";
    break;
  case event_t::condition_wait:
    os << "[" << em.object << ", " << em.object2 << "]";
    break;
  case event_t::thread_create:
  case event_t::thread_join:
    os << "[" << em.target_thread << "]";
  default:
    break;
  }

  return os;
}

inline time_t estimate_time(icount_t const instructions, cpi_t const cpi, freq_t const frequency)
{
  auto const cycles = static_cast<double>(instructions) * cpi;
  auto const period = 1 / static_cast<double>(frequency);
  auto const time = static_cast<std::uint64_t>(std::ceil(1e9 * cycles * period));

  return time_t(time);
}

inline icount_t estimate_instructions(time_t const time, cpi_t const cpi, freq_t const frequency)
{
  auto const cycles = static_cast<double>(time.count()) * static_cast<double>(frequency) * 1e-9;
  auto const instructions = static_cast<icount_t>(cycles / cpi);

  return instructions;
}

} // namespace rhythm

#endif //RHYTHM_COMMON_HPP
