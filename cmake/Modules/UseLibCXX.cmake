#[======================================================================[.rst:
UseLibCXX
----------

Sets toolchain variables and directory properties for using libc++.
Define CMAKE_CXX_STANDARD before including this module.

#]======================================================================]

cmake_minimum_required(VERSION 3.6.0 FATAL_ERROR)
# CMake 3.6 required for
# - CMAKE_<LANG>_INCLUDE_DIRECTORIES variable
# - CMAKE_<LANG>_STANDARD_LIBRARIES variable

find_package(LibCXX 6 REQUIRED)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${LIBCXX_INCLUDE_DIR}")
set(CMAKE_CXX_STANDARD_LIBRARIES "${LIBCXX_CMAKE_CXX_STANDARD_LIBRARIES}")
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>)
if (CMAKE_VERSION VERSION_LESS "3.15.0")
  set(CMAKE_CXX_STANDARD_LIBRARIES "-stdlib=libc++ ${CMAKE_CXX_STANDARD_LIBRARIES}")
else()
  # add_link_options command needs CMake 3.13
  # LINK_LANGUAGE generator expression needs CMake 3.15
  add_link_options($<$<LINK_LANGUAGE:CXX>:-stdlib=libc++>)
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
  #
  # In the meantime, we have a couple of options for dealing with this.
  # 1. Define the preprocessor variable _LIBCPP_DISABLE_DEPRECATION_WARNINGS.
  #    This will silence all deprecation warnings from libc++. References to
  #    deprecated declarations from other sources will still cause a warning
  #    and halt the build. This solution/workaround could potentially silence
  #    valid warnings that would otherwise come from new code that *we*
  #    introduce.
  # 2. Pass the compiler option -Wno-error=deprecated-declarations.
  #    This will prevent references to deprecated declarations from halting
  #    the build. Warnings will not be silenced, but references to deprecated
  #    declarations, even those outside of libc++, will not halt the build.
  # Both of these options have their downsides; however, our decision is more
  # or less made for us, as, should -Werror be added to the compiler options,
  # it would most likely be added after the inclusion of this module, and
  # would therefore appear later in the argument list than option 2's
  # -Wno-error flag, thus overriding it. Additionally, option 2 is much
  # greater in scope, affecting more than just libc++.
  if (CMAKE_VERSION VERSION_LESS "3.12.0")
    set_property(DIRECTORY APPEND PROPERTY $<$<COMPILE_LANGUAGE:CXX>:_LIBCPP_DISABLE_DEPRECATION_WARNINGS>)
  else()
    # add_compile_definitions command needs CMake 3.12
    add_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_LIBCPP_DISABLE_DEPRECATION_WARNINGS>)
  endif()
endif()