#[=======================================================================[.rst:
Findnlohmann_json
-----------------

Finds nlohmann_json. Prioritizes irods-externals if a path is defined.
If nlohmann_json has cmake configuration files, they will be used, but
the return values will be tweaked slightly.

The ``nlohmann_json::nlohmann_json`` :prop_tgt:`IMPORTED` target is
defined, and should be used in lieu of the
``nlohmann_json_INCLUDE_DIRECTORIES`` variable.

#]=======================================================================]

cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR) # VERSION_GREATER_EQUAL

cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0057 NEW)

macro(_nlohmann_json_fix_includes)
  get_target_property(nlohmann_json_INCLUDE_DIRECTORIES nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)  
  list(REMOVE_DUPLICATES nlohmann_json_INCLUDE_DIRECTORIES)
  set_target_properties(
    nlohmann_json::nlohmann_json PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIRECTORIES}"
  )
endmacro()

function(_nlohmann_json_create_target include_dir_var_name)
  if (NOT "${${include_dir_var_name}}" STREQUAL "${include_dir_var_name}-NOTFOUND")

    set(nlohmann_json_INCLUDE_DIRECTORIES "${${include_dir_var_name}}")
    set(nlohmann_json_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIRECTORIES}" PARENT_SCOPE)

    ## Get version number
    include(CheckCPPMacroDefinition)

    set(CMAKE_REQUIRED_INCLUDES "${nlohmann_json_INCLUDE_DIRECTORIES}")
    set(CMAKE_REQUIRED_QUIET "ON")
    set(CMAKE_EXTRA_INCLUDE_FILES "nlohmann/json.hpp")

    # clear cached results if hash of json.hpp has changed
    file(MD5 "${${include_dir_var_name}}/nlohmann/json.hpp" nlohmann_json_HPP_MD5)
    if (NOT DEFINED nlohmann_json_HPP_MD5_LASTRUN OR NOT nlohmann_json_HPP_MD5 STREQUAL nlohmann_json_HPP_MD5_LASTRUN)
      unset(HAVE_nlohmann_json_macro_VERSION_MAJOR CACHE)
      unset(HAVE_nlohmann_json_macro_VERSION_MINOR CACHE)
      unset(HAVE_nlohmann_json_macro_VERSION_PATCH CACHE)
      unset(nlohmann_json_macro_VERSION_MAJOR CACHE)
      unset(nlohmann_json_macro_VERSION_MINOR CACHE)
      unset(nlohmann_json_macro_VERSION_PATCH CACHE)
    endif()
    set(nlohmann_json_HPP_MD5_LASTRUN "${nlohmann_json_HPP_MD5}" CACHE INTERNAL "last value of nlohmann_json_HPP_MD5")

    CHECK_CPP_MACRO_DEFINITION(NLOHMANN_JSON_VERSION_MAJOR nlohmann_json_macro_VERSION_MAJOR LANGUAGE CXX)
    CHECK_CPP_MACRO_DEFINITION(NLOHMANN_JSON_VERSION_MINOR nlohmann_json_macro_VERSION_MINOR LANGUAGE CXX)
    CHECK_CPP_MACRO_DEFINITION(NLOHMANN_JSON_VERSION_PATCH nlohmann_json_macro_VERSION_PATCH LANGUAGE CXX)

    set(nlohmann_json_VERSION "${nlohmann_json_macro_VERSION_MAJOR}.${nlohmann_json_macro_VERSION_MINOR}.${nlohmann_json_macro_VERSION_PATCH}")
    set(nlohmann_json_VERSION "${nlohmann_json_VERSION}" PARENT_SCOPE)

    set(nlohmann_json_CONFIG "non-package:${${include_dir_var_name}}/nlohmann/json.hpp")
    set(nlohmann_json_CONSIDERED_CONFIGS "${nlohmann_json_CONSIDERED_CONFIGS};non-package:${${include_dir_var_name}}/nlohmann/json.hpp" PARENT_SCOPE)
    set(nlohmann_json_CONSIDERED_VERSIONS "${nlohmann_json_CONSIDERED_VERSIONS};${nlohmann_json_VERSION}" PARENT_SCOPE)

    unset("${include_dir_var_name}" CACHE)

    if (DEFINED nlohmann_json_FIND_VERSION)
      if (nlohmann_json_FIND_VERSION_EXACT)
        if (
          nlohmann_json_FIND_VERSION_COUNT EQUAL 1 AND
          NOT nlohmann_json_FIND_VERSION VERSION_EQUAL "${nlohmann_json_macro_VERSION_MAJOR}"
        )
          return()
        elseif (
          nlohmann_json_FIND_VERSION_COUNT EQUAL 2 AND
          NOT nlohmann_json_FIND_VERSION VERSION_EQUAL "${nlohmann_json_macro_VERSION_MAJOR}.${nlohmann_json_macro_VERSION_MINOR}"
        )
          return()
        elseif (NOT nlohmann_json_FIND_VERSION VERSION_EQUAL nlohmann_json_VERSION)
          return()
        endif()
      elseif (nlohmann_json_FIND_VERSION VERSION_GREATER nlohmann_json_VERSION)
        return()
      endif()
    endif()

    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(
      nlohmann_json::nlohmann_json PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIRECTORIES}"
    )
    set(nlohmann_json_CONFIG "${nlohmann_json_CONFIG}" PARENT_SCOPE)
    set(nlohmann_json_CONFIG_unset "ON" PARENT_SCOPE)

  endif()
  unset("${include_dir_var_name}" CACHE)
endfunction()

# Search in IRODS_EXTERNALS path first.
if (DEFINED IRODS_EXTERNALS_FULLPATH_JSON AND NOT TARGET nlohmann_json::nlohmann_json)
  if (DEFINED nlohmann_json_FIND_VERSION)
    if (nlohmann_json_FIND_VERSION_EXACT)
      find_package(
        nlohmann_json "${nlohmann_json_FIND_VERSION}" EXACT
        QUIET
        NO_MODULE
        PATHS "${IRODS_EXTERNALS_FULLPATH_JSON}"
        NO_DEFAULT_PATH
      )
    else()
      find_package(
        nlohmann_json "${nlohmann_json_FIND_VERSION}"
        QUIET
        NO_MODULE
        PATHS "${IRODS_EXTERNALS_FULLPATH_JSON}"
        NO_DEFAULT_PATH
      )
    endif()
  else()
      find_package(
        nlohmann_json
        QUIET
        NO_MODULE
        PATHS "${IRODS_EXTERNALS_FULLPATH_JSON}"
        NO_DEFAULT_PATH
      )
  endif()

  if (NOT TARGET nlohmann_json::nlohmann_json)
    # handle missing configuration files
    find_path(
      nlohmann_json_include_dir
      NAMES "nlohmann/json.hpp"
      PATHS "${IRODS_EXTERNALS_FULLPATH_JSON}/include"
      NO_DEFAULT_PATH
    )
    _nlohmann_json_create_target("nlohmann_json_include_dir")
  endif()
endif()

if (NOT TARGET nlohmann_json::nlohmann_json)
  set(pre_def_nlohmann_json_CONSIDERED_CONFIGS "${nlohmann_json_CONSIDERED_CONFIGS}")
  set(pre_def_nlohmann_json_CONSIDERED_VERSIONS "${nlohmann_json_CONSIDERED_VERSIONS}")

  if (DEFINED nlohmann_json_FIND_VERSION)
    if (nlohmann_json_FIND_VERSION_EXACT)
      find_package(
        nlohmann_json "${nlohmann_json_FIND_VERSION}" EXACT
        QUIET
        NO_MODULE
      )
    else()
      find_package(
        nlohmann_json "${nlohmann_json_FIND_VERSION}"
        QUIET
        NO_MODULE
      )
    endif()
  else()
      find_package(
        nlohmann_json
        QUIET
        NO_MODULE
      )
  endif()

  if(pre_def_nlohmann_json_CONSIDERED_CONFIGS)
    set(nlohmann_json_CONSIDERED_CONFIGS "${pre_def_nlohmann_json_CONSIDERED_CONFIGS};${nlohmann_json_CONSIDERED_CONFIGS}")
    set(nlohmann_json_CONSIDERED_VERSIONS "${pre_def_nlohmann_json_CONSIDERED_VERSIONS};${nlohmann_json_CONSIDERED_VERSIONS}")
  endif()
  unset(pre_def_nlohmann_json_CONSIDERED_CONFIGS)
  unset(pre_def_nlohmann_json_CONSIDERED_VERSIONS)

  if (NOT TARGET nlohmann_json::nlohmann_json)
    # handle missing configuration files
    find_path(
      nlohmann_json_include_dir
      NAMES "nlohmann/json.hpp"
    )
    _nlohmann_json_create_target("nlohmann_json_include_dir")
  endif()
endif()

if (TARGET nlohmann_json::nlohmann_json)
  _nlohmann_json_fix_includes()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  nlohmann_json
  REQUIRED_VARS nlohmann_json_INCLUDE_DIRECTORIES
  VERSION_VAR nlohmann_json_VERSION
  CONFIG_MODE
)

if (nlohmann_json_DIR STREQUAL "nlohmann_json_DIR-NOTFOUND")
  unset(nlohmann_json_DIR CACHE)
endif()

if (DEFINED nlohmann_json_CONFIG_unset)
  unset(nlohmann_json_CONFIG)
  unset(nlohmann_json_CONFIG_unset)
endif()

cmake_policy(POP)