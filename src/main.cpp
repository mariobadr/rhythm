#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "argagg.hpp"

#include "rhythm.hpp"

argagg::parser create_command_line_interface()
{
  return {{{"help", {"-h", "--help"}, "Display help information.", 0},
      {"config", {"-c", "--config"}, "System configuration.", 1},
      {"trace", {"-t", "--trace-manifest"}, "Manifest of all trace files.", 1},
      {"output", {"-o", "--output-dir"}, "Output directory.", 1}}};
}

void print_usage(std::ostream &stream, argagg::parser const &arguments)
{
  argagg::fmt_ostream help(stream);

  help << "Estimate the performance of a parallel program on a parallel architecture.\n\n";
  help << "rhythm [options] ARG [ARG...]\n\n";
  help << arguments;
}

void validate(argagg::parser_results const &options)
{
  if(options["trace"].count() == 0) {
    throw std::runtime_error("Missing path to trace manifest file.");
  }

  if(options["config"].count() == 0) {
    throw std::runtime_error("Missing path to configuration.");
  }

  if(options["output"].count() == 0) {
    throw std::runtime_error("Missing path to output directory.");
  }
}

void setup_loggers()
{
  // Create the logger.
  spdlog::stdout_logger_st("log");

#ifndef NDEBUG
  spdlog::basic_logger_st("rhythm-trace", "rhythm-trace.txt", true);
  spdlog::get("rhythm-trace")->set_pattern("[%n] %v");
#endif
}

int main(int argc, char **argv)
{
  try {
    setup_loggers();

    // Parse the command line.
    auto interface = create_command_line_interface();
    auto const arguments = interface.parse(argc, argv);

    // Check if help was requested.
    if(arguments["help"]) {
      print_usage(std::cout, interface);

      return EXIT_SUCCESS;
    }

    // Make sure we have the required arguments.
    validate(arguments);

    auto const manifest_file = arguments["trace"].as<std::string>();
    auto const config_file = arguments["config"].as<std::string>();
    auto const output_dir = arguments["output"].as<std::string>();

    rhythm::estimate(manifest_file, config_file, output_dir);
  } catch(std::exception const &e) {
    spdlog::get("log")->error("{}", e.what());

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}