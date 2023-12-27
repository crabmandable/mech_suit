cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    mech_suit
    VERSION 0.1.0
    DESCRIPTION "C++ HTTP server lib"
    HOMEPAGE_URL "github.com/crambandable/mech_suit"
    LANGUAGES NONE
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(mech_suit_mech_suit INTERFACE)
add_library(mech_suit::mech_suit ALIAS mech_suit_mech_suit)

set_property(
    TARGET mech_suit_mech_suit PROPERTY
    EXPORT_NAME mech_suit
)

target_include_directories(
    mech_suit_mech_suit ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(mech_suit_mech_suit INTERFACE cxx_std_20)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${mech_suit_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT mech_suit_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of mech_suit"
  )
endif()

include(cmake/dev-mode.cmake)
