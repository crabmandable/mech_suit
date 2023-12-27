cmake_minimum_required(VERSION 3.14)

project(mech_suitTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(mech_suit REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(mech_suit_test source/mech_suit_test.cpp)
target_link_libraries(
    mech_suit_test PRIVATE
    mech_suit::mech_suit
    Catch2::Catch2WithMain
)
target_compile_features(mech_suit_test PRIVATE cxx_std_20)

catch_discover_tests(mech_suit_test)

# ---- End-of-file commands ----

add_folders(Test)
