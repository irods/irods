#[=======================================================================[.rst:
IrodsRunpathDefaults
--------------------

This module sets RPATH-/RUNPATH-related defaults.


#]=======================================================================]

# Allow module to be included multiple times if requested
if (NOT IRODS_ALLOW_RUNPATH_MODULE_MULTIPLE_INCLUDE)
  include_guard(GLOBAL)
endif()

# Allow the new-dtags linker flag to be manually defined
if (NOT DEFINED IRODS_LINKER_FLAG_DTAGS)
  set(IRODS_LINKER_FLAG_DTAGS "-Wl,--enable-new-dtags")
endif()

# Allow the origin linker flag to be manually defined
if (NOT DEFINED IRODS_LINKER_FLAG_ORIGIN)
  set(IRODS_LINKER_FLAG_ORIGIN "-Wl,-z,origin")
endif()

# Helper function. DO NOT CALL DIRECTLY.
function(__add_linker_flag target_type linker_flag)
  # Ensure flag is not already added
  # Also, determine if flags variable is defined and populated
  unset(linker_flags_populated)
  if (DEFINED CMAKE_${target_type}_LINKER_FLAGS_INIT)

    # Get individual args as list
    unset(linker_arg_list)
    string(REGEX REPLACE "[\t ]" ";" linker_arg_list "${CMAKE_${target_type}_LINKER_FLAGS_INIT}")

    # Remove empty entires from args list (for population determination)
    list(REMOVE_ITEM linker_arg_list "")

    # If flag is already present, abort
    if ("${linker_flag}" IN_LIST linker_arg_list)
      return()
    endif()

    # Determine if flags variable is populated
    if (NOT "${linker_arg_list}" STREQUAL "")
      set(linker_flags_populated TRUE)
    endif()
  endif()

  # Add flag
  if (linker_flags_populated)
    set(CMAKE_${target_type}_LINKER_FLAGS_INIT "${CMAKE_${target_type}_LINKER_FLAGS_INIT} ${linker_flag}" PARENT_SCOPE)
  else()
    set(CMAKE_${target_type}_LINKER_FLAGS_INIT "${linker_flag}" PARENT_SCOPE)
  endif()
endfunction()

# Allow includer to request no modifications to linker init flags variables
if (NOT IRODS_RUNPATH_NO_LINKER_FLAGS)
  unset(__flag_type)
  foreach(__flag_type IN ITEMS DTAGS ORIGIN)

    # Allow includer to request specific flag not be added to linker init flags variables
    if (IRODS_RUNPATH_NO_LINKER_FLAG_${__flag_type})
      continue()
    endif()

    unset(__target_type)
    foreach(__target_type IN ITEMS EXE MODULE SHARED)

      # Allow includer to request no modification to specific linker init flags variable
      if (IRODS_RUNPATH_NO_${__target_type}_LINKER_FLAGS)
        continue()
      endif()

      # Allow includer to request specific flag not be added to specific linker init flags variable
      if (IRODS_RUNPATH_NO_${__target_type}_LINKER_FLAG_${__flag_type})
        continue()
      endif()

      # allow linker flags to be manually defined per target type
      unset(__linker_flag)
      if (DEFINED IRODS_${__target_type}_LINKER_FLAG_${__flag_type})
        set(__linker_flag "${IRODS_${__target_type}_LINKER_FLAG_${__flag_type}}")
      else()
        set(__linker_flag "${IRODS_LINKER_FLAG_${__flag_type}}")
      endif()

      # add the flag
      __add_linker_flag(${__target_type} ${__linker_flag})

    endforeach()
  endforeach()
endif()

if (NOT DEFINED CMAKE_SKIP_RPATH)
  set(CMAKE_SKIP_RPATH FALSE)
endif()

if (NOT DEFINED CMAKE_SKIP_BUILD_RPATH)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
endif()

if (NOT DEFINED CMAKE_SKIP_INSTALL_RPATH)
  set(CMAKE_SKIP_INSTALL_RPATH FALSE)
endif()

if (NOT DEFINED CMAKE_INSTALL_RPATH_USE_LINK_PATH)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if (NOT DEFINED CMAKE_BUILD_RPATH_USE_ORIGIN)
  set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
endif()

if (NOT DEFINED IRODS_UNIT_TESTS_BUILD_WITH_INSTALL_RPATH_INIT)
  if (DEFINED CMAKE_BUILD_WITH_INSTALL_RPATH)
    set(IRODS_UNIT_TESTS_BUILD_WITH_INSTALL_RPATH_INIT "${CMAKE_BUILD_WITH_INSTALL_RPATH}")
  else()
    set(IRODS_UNIT_TESTS_BUILD_WITH_INSTALL_RPATH_INIT TRUE)
  endif()
endif()
