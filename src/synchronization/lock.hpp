#ifndef RHYTHM_LOCK_HPP
#define RHYTHM_LOCK_HPP

#include "synchronization-model.hpp"

namespace rhythm {

transition_t acquire(sync_m &sm, thread_t thread_id, std::uint64_t address);

transition_t release(sync_m &sm, thread_t thread_id, std::uint64_t address);

} // namespace rhythm

#endif //RHYTHM_LOCK_HPP
