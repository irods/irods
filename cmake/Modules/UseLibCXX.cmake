#[======================================================================[.rst:
UseLibCXX
----------

Sets toolchain variables and directory properties for using libc++.
Define CMAKE_CXX_STANDARD before including this module.

#]======================================================================]

include_guard(GLOBAL)

cmake_minimum_required(VERSION 3.6.0 FATAL_ERROR)
# CMake 3.6 required for
# - CMAKE_<LANG>_INCLUDE_DIRECTORIES variable
# - CMAKE_<LANG>_STANDARD_LIBRARIES variable

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(IRODS_BUILD_AGAINST_LIBCXX_DEFAULT ON)
else()
  set(IRODS_BUILD_AGAINST_LIBCXX_DEFAULT OFF)
endif()

set(IRODS_BUILD_AGAINST_LIBCXX ${IRODS_BUILD_AGAINST_LIBCXX_DEFAULT} CACHE BOOL "Try to build against libc++ instead of libstdc++.")

if (IRODS_BUILD_AGAINST_LIBCXX)
  include(IrodsExternals)
  IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(CLANG clang13.0.0-0)
  IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(CLANG_RUNTIME clang-runtime13.0.0-0)
  unset(IRODS_PACKAGE_DEPENDENCIES_STRING)
  string(REPLACE ";" ", " IRODS_PACKAGE_DEPENDENCIES_STRING "${IRODS_PACKAGE_DEPENDENCIES_LIST}")

  # 9 required for <filesystem> header
  find_package(LibCXX 9 REQUIRED)

  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${LIBCXX_INCLUDE_DIR}")
  set(CMAKE_CXX_STANDARD_LIBRARIES "${LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES}")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>)
  if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    if (CMAKE_VERSION VERSION_LESS "3.15.0")
      set(CMAKE_CXX_STANDARD_LIBRARIES "-stdlib=libc++ ${CMAKE_CXX_STANDARD_LIBRARIES}")
    else()
      # add_link_options command needs CMake 3.13
      # LINK_LANGUAGE generator expression needs CMake 3.15
      add_link_options($<$<LINK_LANGUAGE:CXX>:-stdlib=libc++>)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(WARNING "Using libc++ with GCC requires that -nodefaultlibs be passed to GCC during the linking step. "
                    "This causes link-time flags such as -static-libgcc to be ignored.")
    find_library(LIBC_LIBRARY NAMES c)
    find_library(LIBGCC_LIBRARY NAMES gcc_s gcc gcc_eh)
    if (LIBGCC_LIBRARY)
      set(CMAKE_CXX_STANDARD_LIBRARIES "${LIBGCC_LIBRARY} ${CMAKE_CXX_STANDARD_LIBRARIES}")
    else()
      set(CMAKE_CXX_STANDARD_LIBRARIES "-lgcc_s ${CMAKE_CXX_STANDARD_LIBRARIES}")
    endif()
    if (LIBC_LIBRARY)
      set(CMAKE_CXX_STANDARD_LIBRARIES "${LIBC_LIBRARY} ${CMAKE_CXX_STANDARD_LIBRARIES}")
    else()
      set(CMAKE_CXX_STANDARD_LIBRARIES "-lc ${CMAKE_CXX_STANDARD_LIBRARIES}")
    endif()
    if (CMAKE_VERSION VERSION_LESS "3.15.0")
      set(CMAKE_CXX_STANDARD_LIBRARIES "-nodefaultlibs ${CMAKE_CXX_STANDARD_LIBRARIES}")
    else()
      add_link_options($<$<LINK_LANGUAGE:CXX>:-nodefaultlibs>)
    endif()

    # nodefaultlibs causes -pthread to become a non-op.
    set(THREADS_PREFER_PTHREAD_FLAG FALSE)
    unset(THREADS_HAVE_PTHREAD_ARG)
  endif()

  # in some versions of CMake, list(INSERT 0) is a non-op on empty lists, so we
  # need to handle the case in which the list is not empty.
  # (list(PREPEND) isn't a thing until CMake 3.15)
  if (NOT CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH "${LIBCXX_LIBRARY_DIR}")
  else()
    list(INSERT 0 CMAKE_INSTALL_RPATH "${LIBCXX_LIBRARY_DIR}")
  endif()
  if (NOT CMAKE_VERSION VERSION_LESS "3.8.0")
    # CMAKE_BUILD_RPATH variable needs CMake 3.8
    if (NOT CMAKE_BUILD_RPATH)
      set(CMAKE_BUILD_RPATH "${LIBCXX_LIBRARY_DIR}")
    else()
      list(INSERT 0 CMAKE_BUILD_RPATH "${LIBCXX_LIBRARY_DIR}")
    endif()
  endif()

  if (LIBCXX_VERSION VERSION_GREATER 10 AND CMAKE_CXX_STANDARD EQUAL 17)
    # C++17 deprecated the explicit template specialization std::allocator<void>
    # and the two-argument version of std::allocator::allocate
    # Starting with 11.0, libc++ enforces these deprecations, which causes
    # warnings from the boost headers.
    # Boost has removed references to the two-argument version of
    # std::allocator::allocate in 1.75, which would solve the problem, but,
    # while the template specification deprecation is not supposed to affect
    # existing code, libc++ 11 causes warnings for references to
    # std::allocator<void>. Maybe a future release of libc++ will fix this.
    # Should we ever switch to C++20 (or newer), std::allocator<void> problem
    # will go away, as the explicit template specialization is removed in C++20.
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-error=deprecated-declarations>)
  endif()
endif()
