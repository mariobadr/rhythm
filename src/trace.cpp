#include "trace.hpp"

#include <set>
#include <sstream>

#include "spdlog/spdlog.h"
#include "zstr.hpp"

namespace rhythm {

// What pthread_t typically is in the pthreads library.
using pthread_t = unsigned long int;

struct trace_row {
  thread_t thread_id = -1;
  std::string call = "";
  pthread_t handle = 0;
  address_t arg1 = 0;
  address_t arg2 = 0;
  size_t barrier_count = 0;
  icount_t instruction_count = 0;

  friend std::istream &operator>>(std::istream &stream, trace_row &row) {
    stream >> row.thread_id;
    stream >> row.call;

    if (row.call == "pthread_create" || row.call == "pthread_join") {
      stream >> row.handle;
    } else {
      stream >> row.arg1;
    }

    stream >> row.instruction_count;

    if (row.call == "pthread_barrier_init") {
      stream >> row.barrier_count;
    } else if (row.call == "pthread_cond_wait") {
      stream >> row.arg2;
    }

    return stream;
  }
};

event_m create_event(trace_row const &row, sync_m &sm,
                     std::map<pthread_t, thread_t> &handles,
                     thread_t &next_create_id) {
  static std::set<std::string> not_supported = {
      "pthread_mutex_trylock",
      "pthread_rwlock_trywrlock",
      "pthread_rwlock_tryrdlock",
      "pthread_spin_trylock",
  };

  if (not_supported.find(row.call) != not_supported.end()) {
    throw std::runtime_error(row.call + " is not supported.");
  }

  if (row.call == "pthread_barrier_init") {
    add_barrier(sm, row.arg1, row.barrier_count);

    return event_m{};
  }

  if (row.call == "pthread_cond_init") {
    add_condition_variable(sm, row.arg1);

    return event_m{};
  }

  static std::set<std::string> lock_inits = {
      "pthread_mutex_init",
      "pthread_rwlock_init",
      "pthread_spin_init",
  };

  if (lock_inits.find(row.call) != lock_inits.end()) {
    add_lock(sm, row.arg1);

    return event_m{};
  }

  if (row.call == "pthread_create") {
    next_create_id++;

    bool was_inserted = false;
    std::map<pthread_t, thread_t>::iterator handle;
    std::tie(handle, was_inserted) =
        handles.emplace(row.handle, next_create_id);
    if (!was_inserted) {
      // This can happen if a thread finishes and another pthread_create call
      // occurs. For now, assume that a join call occurs before the
      // pthread_create, so we will just overwrite the thread ID for this
      // handle.
      handle->second = next_create_id;
    }

    event_m create;

    create.thread_id = row.thread_id;
    create.type = event_t::thread_create;
    create.distance = row.instruction_count;
    create.target_thread = next_create_id;

    add_thread(sm, next_create_id);

    return create;
  }

  static std::set<std::string> lock_calls = {
      "pthread_mutex_lock",    "pthread_mutex_timedlock",
      "pthread_rwlock_wrlock", "pthread_rwlock_timedwrlock",
      "pthread_rwlock_rdlock", "pthread_rwlock_timedrdlock",
      "pthread_spin_lock",
  };

  if (lock_calls.find(row.call) != lock_calls.end()) {
    event_m lock;

    lock.thread_id = row.thread_id;
    lock.type = event_t::lock_acquire;
    lock.object = row.arg1;
    lock.distance = row.instruction_count;

    return lock;
  }

  static std::set<std::string> unlock_calls = {
      "pthread_mutex_unlock",
      "pthread_rwlock_unlock",
      "pthread_spin_unlock",
  };

  if (unlock_calls.find(row.call) != unlock_calls.end()) {
    event_m unlock;

    unlock.thread_id = row.thread_id;
    unlock.type = event_t::lock_release;
    unlock.object = row.arg1;
    unlock.distance = row.instruction_count;

    return unlock;
  }

  if (row.call == "pthread_barrier_wait") {
    event_m barrier;

    barrier.thread_id = row.thread_id;
    barrier.type = event_t::barrier_wait;
    barrier.object = row.arg1;
    barrier.distance = row.instruction_count;

    return barrier;
  }

  if (row.call == "pthread_cond_broadcast") {
    event_m broadcast;

    broadcast.thread_id = row.thread_id;
    broadcast.type = event_t::condition_broadcast;
    broadcast.object = row.arg1;
    broadcast.distance = row.instruction_count;

    update_condition_variable(sm, broadcast);

    return broadcast;
  }

  if (row.call == "pthread_cond_signal") {
    event_m signal;

    signal.thread_id = row.thread_id;
    signal.type = event_t::condition_signal;
    signal.object = row.arg1;
    signal.distance = row.instruction_count;

    update_condition_variable(sm, signal);

    return signal;
  }

  if (row.call == "pthread_cond_wait") {
    event_m wait;

    wait.thread_id = row.thread_id;
    wait.type = event_t::condition_wait;
    wait.object = row.arg1;
    wait.object2 = row.arg2;
    wait.distance = row.instruction_count;

    update_condition_variable(sm, wait);

    return wait;
  }

  if (row.call == "thread_start") {
    event_m start;

    start.thread_id = row.thread_id;
    start.type = event_t::thread_start;
    start.distance = row.instruction_count;

    return start;
  }

  if (row.call == "thread_finish") {
    event_m finish;

    finish.thread_id = row.thread_id;
    finish.type = event_t::thread_finish;
    finish.distance = row.instruction_count;

    return finish;
  }

  if (row.call == "pthread_join") {
    auto const &join_target = handles.find(row.handle);
    event_m join;

    join.thread_id = row.thread_id;
    join.type = event_t::thread_join;
    join.distance = row.instruction_count;
    join.target_thread = join_target->second;

    return join;
  }

  return event_m{};
}

app_m parse_traces(std::string const &manifest_file, sync_m &sm) {
  zstr::ifstream manifest(manifest_file);
  if (!manifest.good()) {
    throw std::runtime_error("Could not load " + manifest_file);
  }

  // We need to associate pthread_t handles with thread IDs.
  std::map<pthread_t, thread_t> handles;
  thread_t next_create_id = 0;

  // Add the master thread.
  add_thread(sm, next_create_id);

  app_m app{};

  std::string file;
  while (manifest >> file) {
    zstr::ifstream trace(file);
    if (!trace.good()) {
      throw std::runtime_error("Could not load " + file);
    } else {
      spdlog::get("log")->info("Loading trace file: {}", file);
    }

    std::string line;
    icount_t instruction_count = 0;

    while (std::getline(trace, line) && !line.empty()) {
      std::istringstream line_stream(line);
      trace_row row;

      if (line_stream >> row) {
        auto emplaced = app.threads.emplace(row.thread_id, row.thread_id);
        auto &tm = emplaced.first->second;

        auto event = create_event(row, sm, handles, next_create_id);
        if (event.type != event_t::unknown) {
          auto const delta = row.instruction_count - instruction_count;
          instruction_count = row.instruction_count;
          event.distance = delta;

          add_event(tm, event);
        }
      }
    }
  }

  return app;
}

} // namespace rhythm