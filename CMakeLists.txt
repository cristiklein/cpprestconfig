cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

option(CPPRESTCONFIG_TESTS "Build tests" OFF)
option(CPPRESTCONFIG_SAMPLES "Build samples" OFF)

project(cpprestconfig CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Boost 1.54 REQUIRED COMPONENTS filesystem)

include(cpplint)
cpplint_add_subdirectory(include)
cpplint_add_subdirectory(src)

set(BUILD_TESTS OFF CACHE BOOL "")
set(CPPREST_EXCLUDE_WEBSOCKETS ON CACHE BOOL "")
add_subdirectory(3rdparty/cpprestsdk/Release EXCLUDE_FROM_ALL)

# Core and main are split. This allows us to link core to main and tests.

# Core library. *.cpp should be added here.
add_library(cpprestconfig
  ./src/cpprestconfig.cc)
target_include_directories(cpprestconfig PUBLIC
  ./include)
target_include_directories(cpprestconfig PRIVATE
  ./3rdparty/spdlog/include)
target_link_libraries(cpprestconfig PRIVATE
  Boost::filesystem
  cpprest)

# Add flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

if(CPPRESTCONFIG_TESTS)
  message("Building tests")

  enable_testing()

  add_subdirectory(3rdparty/googletest)

  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  add_executable(cpprestconfig_tests
    ./src/cpprestconfig_test.cc)

  target_link_libraries(cpprestconfig_tests
    cpprest
    gtest
    gtest_main
    cpprestconfig)

  add_test(
    NAME cpprestconfig_tests
    COMMAND $<TARGET_FILE:cpprestconfig_tests>
  )

endif()

if(CPPRESTCONFIG_SAMPLES)
  message("Building samples")

  add_executable(sample
    ./samples/sample.cc)

  target_link_libraries(sample
    cpprestconfig)

endif()
