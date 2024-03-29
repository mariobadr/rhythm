#ifndef RHYTHM_PIN_TOOL_PTHREAD_EVENTS_HPP
#define RHYTHM_PIN_TOOL_PTHREAD_EVENTS_HPP

/**
 * A list of all the pthread function calls for synchronization.
 */
static const char *events[] = {"pthread_cond_init", "pthread_mutex_init", "pthread_rwlock_init",
    "pthread_spin_init", "pthread_barrier_destroy", "pthread_cond_destroy", "pthread_mutex_destroy",
    "pthread_rwlock_destroy", "pthread_spin_destroy", "pthread_exit", "pthread_barrier_wait",
    "pthread_cond_broadcast", "pthread_cond_signal", "pthread_cond_timedwait", "pthread_cond_wait",
    "pthread_mutex_lock", "pthread_mutex_unlock", "pthread_mutex_timedlock",
    "pthread_mutex_trylock", "pthread_rwlock_wrlock", "pthread_rwlock_timedwrlock",
    "pthread_rwlock_trywrlock", "pthread_rwlock_rdlock", "pthread_rwlock_timedrdlock",
    "pthread_rwlock_tryrdlock", "pthread_rwlock_unlock", "pthread_spin_lock", "pthread_spin_unlock",
    "pthread_spin_trylock", "thread_start", "thread_finish", "pthread_create", "pthread_join"};

/**
 * The number of pthread synchronization function calls.
 */
#define SYNC_CALLS_SIZE 29

/**
 * The number of all pthread function calls.
 */
#define PTHREAD_CALLS_SIZE 33


#endif //RHYTHM_PIN_TOOL_PTHREAD_EVENTS_HPP
