#ifndef RHYTHM_CONTROLLER_HPP
#define RHYTHM_CONTROLLER_HPP

#include "application.hpp"
#include "architecture.hpp"
#include "system-model.hpp"
#include "synchronization-model.hpp"
#include "statistics.hpp"

namespace rhythm {

/**
 * Execute up to the next synchronization event on the critical path.
 */
time_t step(app_m &app, arch_m &arch, sched_m &sched, sync_m &sm, stats_t &stats);

} // namespace rhythm

#endif //RHYTHM_CONTROLLER_HPP
