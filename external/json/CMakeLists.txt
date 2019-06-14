project(
  json
  VERSION 3.6.1
  LANGUAGES CXX
)

add_library(${PROJECT_NAME} INTERFACE)
add_library(nlohmann::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(
  ${PROJECT_NAME}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/json.hpp
)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
