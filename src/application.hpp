#ifndef RHYTHM_APPLICATION_HPP
#define RHYTHM_APPLICATION_HPP

#include <deque>
#include <map>

#include "common.hpp"

#include "spdlog/fmt/ostr.h"

namespace rhythm {

/**
 * Represents a thread as a sequence of events separated by dynamic instruction counts.
 */
struct application_thread {
  thread_t id;
  std::deque<event_m> events;

  /**
   * Constructor.
   */
  explicit application_thread(thread_t tid) : id(tid)
  {
  }
};

/**
 * Represents an application as a collection of threads.
 */
struct app_m {
  std::map<thread_t, application_thread> threads;
};

/**
 * Add a synchronization event to the thread model.
 */
void add_event(application_thread &tm, event_m event);

/**
 * Remove the current event from the thread model.
 *
 * This should only be done when the current event no longer has instructions to execute.
 */
void pop_current_event(application_thread &tm);

/**
 * @return The next synchronization event.
 */
event_m get_current_event(application_thread const &tm);

/**
 * Progress the thread by executing instructions.
 */
void execute(application_thread &tm, icount_t instructions);

template <typename ostream>
ostream &operator<<(ostream &os, application_thread const &tm)
{
  os << "[thread_model] ";
  os << "ID: " << tm.id << ", ";
  os << "Events: " << tm.events.size();

  return os;
}

template <typename ostream>
ostream &operator<<(ostream &os, app_m const &am)
{
  os << "[application_model] ";
  os << "Threads: " << am.threads.size();

  return os;
}

} // namespace rhythm

#endif //RHYTHM_APPLICATION_HPP
