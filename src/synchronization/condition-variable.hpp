#ifndef RHYTHM_CONDITION_VARIABLE_HPP
#define RHYTHM_CONDITION_VARIABLE_HPP

namespace rhythm {

transition_t condition_broadcast(sync_m &sm, thread_t thread_id, std::uint64_t address);

transition_t condition_signal(sync_m &sm, std::uint64_t address);

transition_t
condition_wait(sync_m &sm, thread_t thread_id, std::uint64_t address, std::uint64_t mutex);

} // namespace rhythm

#endif //RHYTHM_CONDITION_VARIABLE_HPP
