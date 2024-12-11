#[=======================================================================[.rst:
Findsystemd
-----------

Find the systemd library (``libsystemd``)

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``systemd::libsystemd``
  The systemd library, if found.

Result and cache variables
^^^^^^^^^^^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``SYSTEMD_FOUND``
  True if libsystemd is found.
``SYSTEMD_INCLUDE_DIRS``
  Include directories for systemd headers. This is also a cache variable
  that can be overridden.
``SYSTEMD_LIBRARIES``
  Libraries to link to libsystemd. This is also a cache variable that can
  be overridden.
``SYSTEMD_COMPILE_OPTIONS``
  Compiler options for use when using libsystemd. This is also a cache
  variable that can be overridden.
``SYSTEMD_LINK_OPTIONS``
  Linker options for use when using libsystemd. This is also a cache
  variable that can be overridden.
``SYSTEMD_LIKELY_VERSION``
  The systemd version number supplied by pkg-config/pkgconf.

This module *may* set the following variable in your project:

``SYSTEMD_VERSION``
  The systemd version number, confirmed to a reasonable degree of
  certainty.

While the CMake documentation recommends against caching the above
variables, we do anyway to provide a clear means of manipulating the
generated import target.

#]=======================================================================]

cmake_policy(PUSH)

# consistent behavior of cache variables for find_* functions
if (POLICY CMP0125)
	cmake_policy(SET CMP0125 NEW)
endif()
# mark_as_advanced() should not create new cache entries
if (POLICY CMP0102)
	cmake_policy(SET CMP0102 NEW)
endif()
cmake_policy(SET CMP0057 NEW) # if(IN_LIST)
if (POLICY CMP0121)
	cmake_policy(SET CMP0121 NEW)
endif()

# check to see if lists are equivalent, ignoring ordering and duplicates
function(_fsd_check_list_equivalence list1 list2 outvar)
	foreach(l1e IN LISTS "${list1}")
		if (NOT l1e IN_LIST "${list2}")
			set("${outvar}" FALSE PARENT_SCOPE)
			return()
		endif()
	endforeach()
	foreach(l2e IN LISTS "${list2}")
		if (NOT l2e IN_LIST "${list1}")
			set("${outvar}" FALSE PARENT_SCOPE)
			return()
		endif()
	endforeach()
	set("${outvar}" TRUE PARENT_SCOPE)
endfunction()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PC_LIBSYSTEMD QUIET libsystemd)
endif()

set(SYSTEMD_INCLUDE_DIRS_DOCSTRING "systemd include directories")
if (SYSTEMD_INCLUDE_DIRS)
	set(SYSTEMD_INCLUDE_DIRS "${SYSTEMD_INCLUDE_DIRS}" CACHE STRING "${SYSTEMD_INCLUDE_DIRS_DOCSTRING}")
else()
	find_path(
		SYSTEMD_INCLUDE_DIR "systemd/sd-daemon.h"
		HINTS "${PC_LIBSYSTEMD_INCLUDE_DIRS}" "${PC_LIBSYSTEMD_INCLUDEDIR}"
		DOC "systemd main include directory"
	)
	mark_as_advanced(FORCE SYSTEMD_INCLUDE_DIR)
	set(_SYSTEMD_INCLUDE_DIRS "${PC_LIBSYSTEMD_INCLUDE_DIRS}")
	if (SYSTEMD_INCLUDE_DIR AND NOT SYSTEMD_INCLUDE_DIR IN_LIST _SYSTEMD_INCLUDE_DIRS)
		list(INSERT _SYSTEMD_INCLUDE_DIRS 0 "${SYSTEMD_INCLUDE_DIR}")
	endif()
	if (NOT _SYSTEMD_INCLUDE_DIRS)
		set(_SYSTEMD_INCLUDE_DIRS "SYSTEMD_INCLUDE_DIRS-NOTFOUND")
	endif()
	set(SYSTEMD_INCLUDE_DIRS "${_SYSTEMD_INCLUDE_DIRS}" CACHE STRING "${SYSTEMD_INCLUDE_DIRS_DOCSTRING}")
endif()

set(SYSTEMD_LIBRARIES_DOCSTRING "systemd libraries")
if (SYSTEMD_LIBRARIES)
	set(SYSTEMD_LIBRARIES "${SYSTEMD_LIBRARIES}" CACHE STRING "${SYSTEMD_LIBRARIES_DOCSTRING}")
else()
	find_library(
		SYSTEMD_LIBRARY
		NAMES systemd libsystemd
		HINTS "${PC_LIBSYSTEMD_LIBRARY_DIRS}" "${PC_LIBSYSTEMD_LIBDIR}"
		DOC "systemd main library"
	)
	mark_as_advanced(FORCE SYSTEMD_LIBRARY)
	set(_SYSTEMD_LIBRARIES "${PC_LIBSYSTEMD_LINK_LIBRARIES}")
	if (SYSTEMD_LIBRARY AND NOT SYSTEMD_LIBRARY IN_LIST _SYSTEMD_LIBRARIES)
		list(INSERT _SYSTEMD_LIBRARIES 0 "${SYSTEMD_LIBRARY}")
	endif()
	if (NOT _SYSTEMD_LIBRARIES)
		set(_SYSTEMD_LIBRARIES "SYSTEMD_LIBRARIES-NOTFOUND")
	endif()
	set(SYSTEMD_LIBRARIES "${_SYSTEMD_LIBRARIES}" CACHE STRING "${SYSTEMD_LIBRARIES_DOCSTRING}")
endif()

set(SYSTEMD_COMPILE_OPTIONS_DOCSTRING "compile options for building with systemd")
if (SYSTEMD_COMPILE_OPTIONS)
	set(SYSTEMD_COMPILE_OPTIONS "${SYSTEMD_COMPILE_OPTIONS}" CACHE STRING "${SYSTEMD_COMPILE_OPTIONS_DOCSTRING}")
else()
	set(_SYSTEMD_COMPILE_OPTIONS "${PC_LIBSYSTEMD_CFLAGS_OTHER}")
	if (NOT _SYSTEMD_COMPILE_OPTIONS)
		set(_SYSTEMD_COMPILE_OPTIONS "")
	endif()
	set(SYSTEMD_COMPILE_OPTIONS "${_SYSTEMD_COMPILE_OPTIONS}" CACHE STRING "${SYSTEMD_COMPILE_OPTIONS_DOCSTRING}")
endif()
mark_as_advanced(FORCE SYSTEMD_COMPILE_OPTIONS)

set(SYSTEMD_LINK_OPTIONS_DOCSTRING "link options for building with systemd")
if (SYSTEMD_LINK_OPTIONS)
	set(SYSTEMD_LINK_OPTIONS "${SYSTEMD_LINK_OPTIONS}" CACHE STRING "${SYSTEMD_LINK_OPTIONS_DOCSTRING}")
else()
	set(_SYSTEMD_LINK_OPTIONS "${PC_LIBSYSTEMD_LDFLAGS_OTHER}")
	if (NOT _SYSTEMD_LINK_OPTIONS)
		set(_SYSTEMD_LINK_OPTIONS "")
	endif()
	set(SYSTEMD_LINK_OPTIONS "${_SYSTEMD_LINK_OPTIONS}" CACHE STRING "${SYSTEMD_LINK_OPTIONS_DOCSTRING}")
endif()
mark_as_advanced(FORCE SYSTEMD_LINK_OPTIONS)

if (PC_LIBSYSTEMD_VERSION)
	set(SYSTEMD_LIKELY_VERSION "${PC_LIBSYSTEMD_VERSION}")

	# try to confirm that we're actually using the systemd pkgconf points us to
	set(_systemd_pc_version_reasonably_certain FALSE)
	if (SYSTEMD_INCLUDE_DIRS AND SYSTEMD_LIBRARIES)
		_fsd_check_list_equivalence(SYSTEMD_INCLUDE_DIRS PC_LIBSYSTEMD_INCLUDE_DIRS _systemd_include_dirs_pc_equiv)
		if (_systemd_include_dirs_pc_equiv)
			_fsd_check_list_equivalence(SYSTEMD_LIBRARIES PC_LIBSYSTEMD_LINK_LIBRARIES _systemd_libs_pc_equiv)
			if (_systemd_libs_pc_equiv)
				set(_systemd_pc_version_reasonably_certain TRUE)
			endif()
		endif()
	endif()
	if (_systemd_pc_version_reasonably_certain)
		set(SYSTEMD_VERSION "${PC_LIBSYSTEMD_VERSION}")
	endif()
endif()

if (SYSTEMD_INCLUDE_DIRS AND SYSTEMD_LIBRARIES)
	add_library(systemd::libsystemd INTERFACE IMPORTED)
	set_target_properties(
		systemd::libsystemd
		PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${SYSTEMD_INCLUDE_DIRS}"
		INTERFACE_LINK_LIBRARIES "${SYSTEMD_LIBRARIES}"
		INTERFACE_COMPILE_OPTIONS "${SYSTEMD_COMPILE_OPTIONS}"
		INTERFACE_LINK_OPTIONS "${SYSTEMD_LINK_OPTIONS}"
	)
	if (SYSTEMD_VERSION)
		set_target_properties(
			systemd::libsystemd
			PROPERTIES
			VERSION "${SYSTEMD_VERSION}"
		)
	endif()
endif()

include(FindPackageHandleStandardArgs)
if (SYSTEMD_VERSION)
	set(_fphsa_vv1 "VERSION_VAR")
	set(_fphsa_vv2 "SYSTEMD_VERSION")
else()
	unset(_fphsa_vv1)
	unset(_fphsa_vv2)
endif()
find_package_handle_standard_args(
  systemd
  REQUIRED_VARS SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES
  ${_fphsa_vv1} ${_fphsa_vv2}
)

cmake_policy(POP)
