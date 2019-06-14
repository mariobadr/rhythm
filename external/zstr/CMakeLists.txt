project(
  zstr
  VERSION 1.0.1
  LANGUAGES CXX
)

find_package(ZLIB REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(
  ${PROJECT_NAME}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/strict_fstream.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/zstr.hpp
)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  ${PROJECT_NAME}
  INTERFACE
    ${ZLIB_LIBRARIES}
)
