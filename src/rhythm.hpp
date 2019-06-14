#ifndef RHYTHM_RHYTHM_HPP
#define RHYTHM_RHYTHM_HPP

#include <string>

namespace rhythm {

/**
 * Estimate performance for an application, system, and architecture.
 */
void estimate(std::string const &manifest_file,
    std::string const &config_file,
    std::string const &output_dir);

} // namespace rhythm

#endif //RHYTHM_RHYTHM_HPP
