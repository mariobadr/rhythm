cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(
  rhythm
  VERSION 0.0.1
)

# Globally set that all executables be compiled into a 'bin' directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Warning flags to use for different compilers.
set(RHYTHM_MSVC_WARNING_FLAGS /W4 /WX)
set(RHYTHM_GCC_WARNING_FLAGS -Wall -Wextra -Wnon-virtual-dtor -pedantic -Wold-style-cast -Wcast-align
  -Woverloaded-virtual -Wconversion)

# Add libraries from external sources.
add_subdirectory(external)

add_executable(
  ${PROJECT_NAME}
  src/synchronization/barrier.cpp
  src/synchronization/barrier.hpp
  src/synchronization/condition-variable.cpp
  src/synchronization/condition-variable.hpp
  src/synchronization/lock.cpp
  src/synchronization/lock.hpp
  src/architecture.hpp
  src/architecture.cpp
  src/application.cpp
  src/application.hpp
  src/common.hpp
  src/controller.cpp
  src/controller.hpp
  src/main.cpp
  src/rhythm.cpp
  src/rhythm.hpp
  src/statistics.cpp
  src/statistics.hpp
  src/synchronization-model.cpp
  src/synchronization-model.hpp
  src/system-model.cpp
  src/system-model.hpp
  src/trace.hpp
  src/trace.cpp
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
    argagg::argagg
    nlohmann::json
    spdlog::spdlog
    zstr::zstr
)

# Require the C++14 standard.
set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
)

# Enable the compiler-specific warning flags.
if(MSVC)
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
      ${RHYTHM_MSVC_WARNING_FLAGS}
  )
else()
  target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
      ${RHYTHM_GCC_WARNING_FLAGS}
  )
endif()
