#[=======================================================================[.rst:
IrodsExternals
--------------


#]=======================================================================]

include_guard(GLOBAL)

if (NOT IRODS_EXTERNALS_PACKAGE_ROOT)
  set(IRODS_EXTERNALS_PACKAGE_ROOT "/opt/irods-externals" CACHE STRING "Choose the location of iRODS external packages." FORCE)
  message(STATUS "Setting unspecified IRODS_EXTERNALS_PACKAGE_ROOT to '${IRODS_EXTERNALS_PACKAGE_ROOT}'. This is the correct setting for normal builds.")
endif()

macro(IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH VAR DEPENDENCY_SUBDIRECTORY)
  if (IS_DIRECTORY ${IRODS_EXTERNALS_FULLPATH_${VAR}})
    message(STATUS "Using user-specified value for IRODS_EXTERNALS_FULLPATH_${VAR}: ${IRODS_EXTERNALS_FULLPATH_${VAR}}")
  else()
    if (NOT IS_DIRECTORY ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
      message(FATAL_ERROR "${VAR} not found at ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY}")
    endif()
    set(IRODS_EXTERNALS_FULLPATH_${VAR} ${IRODS_EXTERNALS_PACKAGE_ROOT}/${DEPENDENCY_SUBDIRECTORY})
  endif()
endmacro()

macro(IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST VAR DEPENDENCY_SUBDIRECTORY)
  IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(${VAR} ${DEPENDENCY_SUBDIRECTORY})
  list(APPEND IRODS_PACKAGE_DEPENDENCIES_LIST irods-externals-${DEPENDENCY_SUBDIRECTORY})
endmacro()

