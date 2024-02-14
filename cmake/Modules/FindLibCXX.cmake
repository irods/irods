#[======================================================================[.rst:
FindLibCXX
----------

This module is deprecated and will be removed in a future release of iRODS

Result variables
^^^^^^^^^^^^^^^^

This module will define the following variables:

``LIBCXX_FOUND``
  true if libc++ headers and libraries were found
``LIBCXX_LIBRARIES``
  libc++ libraries to be linked
``LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES``
  LIBCXX_LIBRARIES, but formatted like CMAKE_CXX_STANDARD_LIBRARIES
``LIBCXX_LIBRARY_DIR``
  directory containing libc++
``LIBCXX_VERSION_INT``
  definition of _LIBCPP_VERSION from __config
``LIBCXX_VERSION``
  version number derived from LIBCXX_VERSION_INT

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LIBCXX_INCLUDE_DIR``
  the directory containing libc++ headers
``LIBCXX_LIBRARY``
  libc++ library
``LIBCXX_LIBCXXABI_LIBRARY``
  libc++abi library (if found)

#]======================================================================]

string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
if (NOT PROJECT_NAME_UPPER STREQUAL "IRODS")
  message(DEPRECATION "The LibCXX CMake find module is deprecated and will be removed in a future relase of iRODS.")
endif()

get_filename_component(__find_libcxx_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)

# We need to set some variables to manipulate find_library behavior
set(PRE_CMAKE_FIND_LIBRARY_PREFIXES "${CMAKE_FIND_LIBRARY_PREFIXES}")
set(PRE_CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
# These libraries always start with lib, even on platforms where libraries
# usually do not.
set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
# our externals package for libc++ might not have unversioned so symlinks
list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_SHARED_LIBRARY_SUFFIX}.1")
# we will need this later
set(LIBCXX_CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")

# Search in IRODS_EXTERNALS paths first.
# This looks a bit weird because we want to prioritize CLANG_RUNTIME over
# CLANG, but we don't want to use headers from CLANG_RUNTIME if we didn't find
# the library there. Likewise, we don't want to use headers from CLANG if we
# didn't find the library in either location. This means we also have to check
# the location of already-defined LIBCXX_LIBRARY.
# If we were going to attempt to derive the location of the library based on
# the CMake compiler variables (potentially defined on the command line), we'd
# do it here. But we're not. At least for the time being. If the developer is
# specifying the compiler manually, they know enough to be able to specify
# the libcxx variables theirself as well.
if (DEFINED IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME)
    find_library(LIBCXX_LIBRARY NAMES c++
        PATHS
        "${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib"
        NO_DEFAULT_PATH)
    if (NOT LIBCXX_LIBRARY STREQUAL LIBCXX_LIBRARY-NOTFOUND)
        get_filename_component(_irodsext_abs_libdir "${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib" ABSOLUTE)
        get_filename_component(_found_abs_lib "${LIBCXX_LIBRARY}" ABSOLUTE)
        get_filename_component(_found_abs_libdir "${_found_abs_lib}" DIRECTORY)
        if (_irodsext_abs_libdir STREQUAL _found_abs_libdir)
            set(libcxx_found_in_irodsext_clangrt TRUE)
            find_path(LIBCXX_INCLUDE_DIR NAMES __config
                PATHS
                "${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/include/c++/v1"
                NO_DEFAULT_PATH)
        endif()
        unset(_irodsext_abs_libdir)
        unset(_found_abs_lib)
        unset(_found_abs_libdir)
    endif()
endif()
# we only look for versioned libc++ soname in CLANG_RUNTIME dir
# restore default value for CMAKE_FIND_LIBRARY_SUFFIXES
set(CMAKE_FIND_LIBRARY_SUFFIXES "${PRE_CMAKE_FIND_LIBRARY_SUFFIXES}")
if (DEFINED IRODS_EXTERNALS_FULLPATH_CLANG)
    find_library(LIBCXX_LIBRARY NAMES c++
        PATHS
        "${IRODS_EXTERNALS_FULLPATH_CLANG}/lib"
        NO_DEFAULT_PATH)
    if (NOT LIBCXX_LIBRARY STREQUAL LIBCXX_LIBRARY-NOTFOUND)
        get_filename_component(_irodsext_abs_libdir "${IRODS_EXTERNALS_FULLPATH_CLANG}/lib" ABSOLUTE)
        get_filename_component(_found_abs_lib "${LIBCXX_LIBRARY}" ABSOLUTE)
        get_filename_component(_found_abs_libdir "${_found_abs_lib}" DIRECTORY)
        if (libcxx_found_in_irodsext_clangrt OR _irodsext_abs_libdir STREQUAL _found_abs_libdir)
            find_path(LIBCXX_INCLUDE_DIR NAMES __config
                PATHS
                "${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1"
                NO_DEFAULT_PATH)
        endif()
        unset(_irodsext_abs_libdir)
        unset(_found_abs_lib)
        unset(_found_abs_libdir)
    endif()
endif()
unset(libcxx_found_in_irodsext_clangrt)
find_library(LIBCXX_LIBRARY NAMES c++)
find_path(LIBCXX_INCLUDE_DIR NAMES __config
    PATH_SUFFIXES
    "c++/v1")

if (NOT LIBCXX_LIBRARY STREQUAL LIBCXX_LIBRARY-NOTFOUND)
    get_filename_component(LIBCXX_LIBRARY_DIR "${LIBCXX_LIBRARY}" DIRECTORY)
    set(LIBCXX_LIBRARIES "${LIBCXX_LIBRARY}")
    set(LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES "\"${LIBCXX_LIBRARY}\"")

    # If a libc++abi lives next to libc++, use it.
    # I wish I could avoid this, but symbols from libc++abi do end up
    # referenced in the final output files.
    # We only care about libc++abi if it lives next to libc++ or if it was
    # already specified. Otherwise, the linker will find it on its own.
    # For this search, versioned sonames are candidates again
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${LIBCXX_CMAKE_FIND_LIBRARY_SUFFIXES}")
    find_library(LIBCXX_LIBCXXABI_LIBRARY NAMES c++abi
        HINTS
        ${LIBCXX_LIBRARY_DIR}
        NO_DEFAULT_PATH)
    if (LIBCXX_LIBCXXABI_LIBRARY STREQUAL LIBCXX_LIBCXXABI_LIBRARY-NOTFOUND)
        unset(LIBCXX_LIBCXXABI_LIBRARY)
    else()
        list(INSERT LIBCXX_LIBRARIES 0 "${LIBCXX_LIBCXXABI_LIBRARY}")
        set(LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES "\"${LIBCXX_LIBCXXABI_LIBRARY}\" ${LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES}")
    endif()

    # Get version number
    if (NOT LIBCXX_INCLUDE_DIR STREQUAL LIBCXX_INCLUDE_DIR-NOTFOUND)
        include(CheckCPPMacroDefinition)

        # set some variables to manipulate CHECK_CPP_MACRO_DEFINITION behavior
        set(PRE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        set(PRE_CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
        set(PRE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
        set(PRE_CMAKE_REQUIRED_QUIET "${CMAKE_REQUIRED_QUIET}")
        set(PRE_CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_EXTRA_INCLUDE_FILES}")
        set(PRE_CMAKE_REQUIRED_CMAKE_FLAGS "${CMAKE_REQUIRED_CMAKE_FLAGS}")
        set(CMAKE_REQUIRED_FLAGS "-nostdinc++")
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        set(CMAKE_REQUIRED_QUIET ${LibCXX_FIND_QUIETLY})
        set(CMAKE_EXTRA_INCLUDE_FILES "__config")
        list(APPEND CMAKE_REQUIRED_CMAKE_FLAGS "-DCMAKE_CXX_STANDARD_LIBRARIES=${LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES}")
        list(APPEND CMAKE_REQUIRED_CMAKE_FLAGS "-DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${LIBCXX_INCLUDE_DIR}")
        list(APPEND CMAKE_REQUIRED_CMAKE_FLAGS "-DCMAKE_BUILD_RPATH=${LIBCXX_LIBRARY_DIR}")

        # clear cached results if hash of __config has changed
        file(MD5 "${LIBCXX_INCLUDE_DIR}/__config" LIBCXX_CONFIG_MD5)
        if (NOT DEFINED LIBCXX_CONFIG_MD5_LASTRUN OR NOT LIBCXX_CONFIG_MD5 STREQUAL LIBCXX_CONFIG_MD5_LASTRUN)
            unset(HAVE_LIBCXX_VERSION_INT CACHE)
            unset(LIBCXX_VERSION CACHE)
            unset(LIBCXX_VERSION_INT CACHE)
        endif()
        set(LIBCXX_CONFIG_MD5_LASTRUN "${LIBCXX_CONFIG_MD5}" CACHE INTERNAL "last value of LIBCXX_CONFIG_MD5")

        CHECK_CPP_MACRO_DEFINITION(_LIBCPP_VERSION LIBCXX_VERSION_INT LANGUAGE CXX)

        # restore previous values
        set(CMAKE_REQUIRED_FLAGS "${PRE_CMAKE_REQUIRED_FLAGS}")
        set(CMAKE_REQUIRED_INCLUDES "${PRE_CMAKE_REQUIRED_INCLUDES}")
        set(CMAKE_REQUIRED_LIBRARIES "${PRE_CMAKE_REQUIRED_LIBRARIES}")
        set(CMAKE_REQUIRED_QUIET "${PRE_CMAKE_REQUIRED_QUIET}")
        set(CMAKE_EXTRA_INCLUDE_FILES "${PRE_CMAKE_EXTRA_INCLUDE_FILES}")
        set(CMAKE_REQUIRED_CMAKE_FLAGS "${PRE_CMAKE_REQUIRED_CMAKE_FLAGS}")
        unset(PRE_CMAKE_REQUIRED_FLAGS)
        unset(PRE_CMAKE_REQUIRED_INCLUDES)
        unset(PRE_CMAKE_REQUIRED_LIBRARIES)
        unset(PRE_CMAKE_REQUIRED_QUIET)
        unset(PRE_CMAKE_EXTRA_INCLUDE_FILES)
        unset(PRE_CMAKE_REQUIRED_CMAKE_FLAGS)

        # Get version number from int
        if (LIBCXX_VERSION_INT MATCHES "^[0-9][0-9][0-9][0-9]+$")
            math(EXPR LIBCXX_VERSION_MAJOR "${LIBCXX_VERSION_INT} / 1000")
            math(EXPR LIBCXX_VERSION_MINOR "(${LIBCXX_VERSION_INT} / 100) - (${LIBCXX_VERSION_MAJOR} * 10)")
            math(EXPR LIBCXX_VERSION_PATCH "${LIBCXX_VERSION_INT} - (${LIBCXX_VERSION_MAJOR} * 1000) - (${LIBCXX_VERSION_MINOR} * 100)")
            set(LIBCXX_VERSION "${LIBCXX_VERSION_MAJOR}.${LIBCXX_VERSION_MINOR}.${LIBCXX_VERSION_PATCH}" CACHE INTERNAL "libc++ version (derived from LIBCXX_VERSION_INT)")
        endif()
    endif()
endif()

# restore previous values
set(CMAKE_FIND_LIBRARY_PREFIXES "${PRE_CMAKE_FIND_LIBRARY_PREFIXES}")
set(CMAKE_FIND_LIBRARY_SUFFIXES "${PRE_CMAKE_FIND_LIBRARY_SUFFIXES}")
unset(PRE_CMAKE_FIND_LIBRARY_PREFIXES)
unset(PRE_CMAKE_FIND_LIBRARY_SUFFIXES)
unset(LIBCXX_CMAKE_FIND_LIBRARY_SUFFIXES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibCXX
    REQUIRED_VARS LIBCXX_LIBRARY LIBCXX_INCLUDE_DIR LIBCXX_LIBRARIES
    VERSION_VAR LIBCXX_VERSION)