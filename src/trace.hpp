#ifndef RHYTHM_TRACE_HPP
#define RHYTHM_TRACE_HPP

#include <string>

#include "application.hpp"
#include "synchronization-model.hpp"

namespace rhythm {

/**
 * Parse a trace and produce the resulting app_m.
 *
 * The sync_m will be updated with synchronization objects found in the trace.
 */
app_m parse_traces(std::string const &file, sync_m &sm);

} // namespace rhythm

#endif //RHYTHM_TRACE_HPP
