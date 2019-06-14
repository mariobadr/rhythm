#ifndef RHYTHM_STATISTICS_HPP
#define RHYTHM_STATISTICS_HPP

#include <map>

#include "common.hpp"
#include "synchronization-model.hpp"
#include "system-model.hpp"

namespace rhythm {

/**
 * Time spent while in each thread_status.
 */
struct status_tracker {
  std::map<thread_status, time_t> times;
};

/**
 * Time spent waiting on different synchronization events.
 */
struct sync_tracker {
  event_m last_event;
  std::map<address_t, time_t> lock_wait_times;
  std::map<address_t, time_t> barrier_wait_times;
  std::map<address_t, time_t> condition_wait_times;
};

/**
 * A collection of performance metrics that are estimated by Rhythm.
 */
struct stats_t {
  /**
   * Total run time of the application.
   */
  time_t total_time{0};

  /**
   * Total run time of each thread.
   */
  std::map<thread_t, time_t> run_time;

  /**
   * The run time per thread, further divided by thread_status.
   */
  std::map<thread_t, status_tracker> status_time;

  /**
   * The waiting time per thread, further divided by the synchronization object being waited on.
   */
  std::map<thread_t, sync_tracker> sync_time;
};

/**
 * Update the performance metrics based on how much time has elapsed.
 */
void update(stats_t &stats, time_t elapsed, event_m const &event, sync_m const &sm);

/**
 * Print the stats as files to an output directory.
 */
void print(stats_t const &stats, std::string const &output_directory);

} // namespace rhythm

#endif //RHYTHM_STATISTICS_HPP
