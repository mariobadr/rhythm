#include "application.hpp"

#include <cassert>

namespace rhythm {

void add_event(application_thread &tm, event_m event)
{
  assert(event.type != event_t::unknown);

  tm.events.push_back(event);
}

void pop_current_event(application_thread &tm)
{
  assert(!tm.events.empty());

  tm.events.pop_front();
}

event_m get_current_event(application_thread const &tm)
{
  assert(!tm.events.empty());

  return tm.events.front();
}

void execute(application_thread &tm, icount_t instructions)
{
  assert(!tm.events.empty());

  if(tm.events.front().distance >= instructions) {
    tm.events.front().distance -= instructions;
  } else {
    // Due to floating point, estimating instructions from time can cause some off-by-a-little errors so we ensure that
    // the instruction count remains non-negative.
    tm.events.front().distance = 0;
  }
}
} // namespace rhythm
