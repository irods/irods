#[=======================================================================[.rst:
FindCatch2
-----------------

Finds Catch2. Prioritizes irods-externals if a path is defined.
If Catch2 has cmake configuration files, they will be used, but
the return values will be tweaked slightly.

The ``Catch2::Catch2`` :prop_tgt:`IMPORTED` target is
defined, and should be used in lieu of the
``Catch2_INCLUDE_DIRECTORIES`` variable.

#]=======================================================================]

cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR) # VERSION_GREATER_EQUAL

cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0057 NEW)

macro(_Catch2_fix_includes)
  get_target_property(Catch2_INCLUDE_DIRECTORIES Catch2::Catch2 INTERFACE_INCLUDE_DIRECTORIES)  
  list(REMOVE_DUPLICATES Catch2_INCLUDE_DIRECTORIES)
  set_target_properties(
    Catch2::Catch2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Catch2_INCLUDE_DIRECTORIES}"
  )
endmacro()

function(_Catch2_create_target include_dir_var_name)
  if (NOT "${${include_dir_var_name}}" STREQUAL "${include_dir_var_name}-NOTFOUND")

    set(Catch2_INCLUDE_DIRECTORIES "${${include_dir_var_name}}")
    set(Catch2_INCLUDE_DIRECTORIES "${Catch2_INCLUDE_DIRECTORIES}" PARENT_SCOPE)

    ## Get version number
    include(CheckCPPMacroDefinition)

    set(CMAKE_REQUIRED_INCLUDES "${Catch2_INCLUDE_DIRECTORIES}")
    set(CMAKE_REQUIRED_QUIET "ON")
    set(CMAKE_EXTRA_INCLUDE_FILES "catch2/catch.hpp")

    # clear cached results if hash of json.hpp has changed
    file(MD5 "${${include_dir_var_name}}/catch2/catch.hpp" Catch2_HPP_MD5)
    if (NOT DEFINED Catch2_HPP_MD5_LASTRUN OR NOT Catch2_HPP_MD5 STREQUAL Catch2_HPP_MD5_LASTRUN)
      unset(HAVE_Catch2_macro_VERSION_MAJOR CACHE)
      unset(HAVE_Catch2_macro_VERSION_MINOR CACHE)
      unset(HAVE_Catch2_macro_VERSION_PATCH CACHE)
      unset(Catch2_macro_VERSION_MAJOR CACHE)
      unset(Catch2_macro_VERSION_MINOR CACHE)
      unset(Catch2_macro_VERSION_PATCH CACHE)
    endif()
    set(Catch2_HPP_MD5_LASTRUN "${Catch2_HPP_MD5}" CACHE INTERNAL "last value of Catch2_HPP_MD5")

    CHECK_CPP_MACRO_DEFINITION(CATCH_VERSION_MAJOR Catch2_macro_VERSION_MAJOR LANGUAGE CXX)
    CHECK_CPP_MACRO_DEFINITION(CATCH_VERSION_MINOR Catch2_macro_VERSION_MINOR LANGUAGE CXX)
    CHECK_CPP_MACRO_DEFINITION(CATCH_VERSION_PATCH Catch2_macro_VERSION_PATCH LANGUAGE CXX)

    set(Catch2_VERSION "${Catch2_macro_VERSION_MAJOR}.${Catch2_macro_VERSION_MINOR}.${Catch2_macro_VERSION_PATCH}")
    set(Catch2_VERSION "${Catch2_VERSION}" PARENT_SCOPE)

    set(Catch2_CONFIG "non-package:${${include_dir_var_name}}/catch2/catch.hpp")
    set(Catch2_CONSIDERED_CONFIGS "${Catch2_CONSIDERED_CONFIGS};non-package:${${include_dir_var_name}}/catch2/catch.hpp" PARENT_SCOPE)
    set(Catch2_CONSIDERED_VERSIONS "${Catch2_CONSIDERED_VERSIONS};${Catch2_VERSION}" PARENT_SCOPE)

    unset("${include_dir_var_name}" CACHE)

    if (DEFINED Catch2_FIND_VERSION)
      if (Catch2_FIND_VERSION_EXACT)
        if (
          Catch2_FIND_VERSION_COUNT EQUAL 1 AND
          NOT Catch2_FIND_VERSION VERSION_EQUAL "${Catch2_macro_VERSION_MAJOR}"
        )
          return()
        elseif (
          Catch2_FIND_VERSION_COUNT EQUAL 2 AND
          NOT Catch2_FIND_VERSION VERSION_EQUAL "${Catch2_macro_VERSION_MAJOR}.${Catch2_macro_VERSION_MINOR}"
        )
          return()
        elseif (NOT Catch2_FIND_VERSION VERSION_EQUAL Catch2_VERSION)
          return()
        endif()
      elseif (Catch2_FIND_VERSION VERSION_GREATER Catch2_VERSION)
        return()
      endif()
    endif()

    add_library(Catch2::Catch2 INTERFACE IMPORTED)
    set_target_properties(
      Catch2::Catch2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Catch2_INCLUDE_DIRECTORIES}"
    )
    set(Catch2_CONFIG "${Catch2_CONFIG}" PARENT_SCOPE)
    set(Catch2_CONFIG_unset "ON" PARENT_SCOPE)

  endif()
  unset("${include_dir_var_name}" CACHE)
endfunction()

# Search in IRODS_EXTERNALS path first.
if (DEFINED IRODS_EXTERNALS_FULLPATH_CATCH2 AND NOT TARGET Catch2::Catch2)
  if (DEFINED Catch2_FIND_VERSION)
    if (Catch2_FIND_VERSION_EXACT)
      find_package(
        Catch2 "${Catch2_FIND_VERSION}" EXACT
        QUIET
        NO_MODULE
        PATHS "${IRODS_EXTERNALS_FULLPATH_CATCH2}"
        NO_DEFAULT_PATH
      )
    else()
      find_package(
        Catch2 "${Catch2_FIND_VERSION}"
        QUIET
        NO_MODULE
        PATHS "${IRODS_EXTERNALS_FULLPATH_CATCH2}"
        NO_DEFAULT_PATH
      )
    endif()
  else()
      find_package(
        Catch2
        QUIET
        NO_MODULE
        PATHS "${IRODS_EXTERNALS_FULLPATH_CATCH2}"
        NO_DEFAULT_PATH
      )
  endif()

  if (TARGET Catch2::Catch2)
    set(CATCH2_NO_CMAKE NO)
  else()
    # handle missing configuration files
    find_path(
      catch2_include_dir
      NAMES "catch2/catch.hpp"
      PATHS "${IRODS_EXTERNALS_FULLPATH_CATCH2}/include"
      NO_DEFAULT_PATH
    )
    _Catch2_create_target("catch2_include_dir")
    if (TARGET Catch2::Catch2)
      set(CATCH2_NO_CMAKE YES)
    endif()
  endif()
endif()

if (NOT TARGET Catch2::Catch2)
  set(pre_def_Catch2_CONSIDERED_CONFIGS "${Catch2_CONSIDERED_CONFIGS}")
  set(pre_def_Catch2_CONSIDERED_VERSIONS "${Catch2_CONSIDERED_VERSIONS}")

  if (DEFINED Catch2_FIND_VERSION)
    if (Catch2_FIND_VERSION_EXACT)
      find_package(
        Catch2 "${Catch2_FIND_VERSION}" EXACT
        QUIET
        NO_MODULE
      )
    else()
      find_package(
        Catch2 "${Catch2_FIND_VERSION}"
        QUIET
        NO_MODULE
      )
    endif()
  else()
      find_package(
        Catch2
        QUIET
        NO_MODULE
      )
  endif()

  if(pre_def_Catch2_CONSIDERED_CONFIGS)
    set(Catch2_CONSIDERED_CONFIGS "${pre_def_Catch2_CONSIDERED_CONFIGS};${Catch2_CONSIDERED_CONFIGS}")
    set(Catch2_CONSIDERED_VERSIONS "${pre_def_Catch2_CONSIDERED_VERSIONS};${Catch2_CONSIDERED_VERSIONS}")
  endif()
  unset(pre_def_Catch2_CONSIDERED_CONFIGS)
  unset(pre_def_Catch2_CONSIDERED_VERSIONS)

  if (TARGET Catch2::Catch2)
    set(CATCH2_NO_CMAKE NO)
  else()
    # handle missing configuration files
    find_path(
      catch2_include_dir
      NAMES "catch2/catch.hpp"
    )
    _Catch2_create_target("catch2_include_dir")
    if (TARGET Catch2::Catch2)
      set(CATCH2_NO_CMAKE YES)
    endif()
  endif()
endif()

if (TARGET Catch2::Catch2)
  _Catch2_fix_includes()
endif()

if (NOT DEFINED CATCH2_NO_CMAKE)
  set(CATCH2_NO_CMAKE YES)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Catch2
  REQUIRED_VARS Catch2_INCLUDE_DIRECTORIES
  VERSION_VAR Catch2_VERSION
  CONFIG_MODE
)

if (Catch2_DIR STREQUAL "Catch2_DIR-NOTFOUND")
  unset(Catch2_DIR CACHE)
endif()

if (DEFINED Catch2_CONFIG_unset)
  unset(Catch2_CONFIG)
  unset(Catch2_CONFIG_unset)
endif()

cmake_policy(POP)