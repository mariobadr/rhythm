#ifndef RHYTHM_ARCHITECTURE_HPP
#define RHYTHM_ARCHITECTURE_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "common.hpp"

namespace rhythm {

/**
 * Represents a core type that can be found in a multiprocessor.
 */
struct core_t {
  /**
   * The CPI rate that a thread can run at on this type of core.
   */
  std::map<thread_t, cpi_t> cpi_rates;

  /**
   * The available frequencies that this type of core can operate at.
   */
  std::vector<freq_t> frequencies;
};

/**
 * Models a physical or virtual core.
 */
struct core_m {
  /**
   * Create a core based on the core type and set its initial frequency to the first available frequency level.
   */
  explicit core_m(core_t const &t) : type(t), frequency(t.frequencies.at(0))
  {
  }

  /**
   * The type of this core.
   */
  core_t const &type;

  /**
   * The frequency that this core is operating at.
   */
  freq_t frequency;
};

/**
 * Models a multiprocessor as a collection of cores, where each core has a certain type.
 */
struct arch_m {
  /**
   * The different types of cores found in this architecture.
   */
  std::map<std::string, core_t> core_types;

  /**
   * The physical or virtual cores found in this multiprocessor.
   */
  std::vector<core_m> cores;
};

/**
 * Parse a configuration file and produce the resulting arch_m.
 */
arch_m parse_config_file(std::string const &file);

} // namespace rhythm

#endif //RHYTHM_ARCHITECTURE_HPP
