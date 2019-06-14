#ifndef RHYTHM_BARRIER_HPP
#define RHYTHM_BARRIER_HPP

#include "synchronization-model.hpp"

namespace rhythm {

transition_t barrier_wait(sync_m &sm, thread_t thread_id, std::uint64_t address);
}

#endif //RHYTHM_BARRIER_HPP
