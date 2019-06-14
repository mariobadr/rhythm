#include "architecture.hpp"

#include <cassert>
#include <fstream>

#include "json.hpp"

namespace rhythm {

arch_m parse_config_file(std::string const &file)
{
  auto stream = std::ifstream(file);
  auto input = nlohmann::json::parse(stream);

  arch_m arch{};

  for(auto const &core_type_config : input["architecture"]["core.types"]) {
    core_t new_core_type{};

    for(auto const &thread : core_type_config["threads"]) {
      thread_t const thread_id = thread["tid"];
      cpi_t const cpi_rate = thread["cpi.rate"];

      new_core_type.cpi_rates.emplace(thread_id, cpi_rate);
    }

    for(auto const &level: core_type_config["frequency.levels"]) {
      new_core_type.frequencies.push_back(level["frequency"]);
    }

    std::string const core_type_id = core_type_config["id"];
    arch.core_types.emplace(core_type_id, std::move(new_core_type));
  }

  std::deque<std::string> const cores = input["architecture"]["cores"];
  for(auto const &core_type_id : cores) {
    arch.cores.emplace_back(arch.core_types.at(core_type_id));
  }

  return arch;
}

} // namespace rhythm