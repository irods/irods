cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets
# CURL::libcurl target
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
include(RequireOutOfSourceBuild)

option(IRODS_DISABLE_COMPILER_OPTIMIZATIONS "Disables compiler optimizations by setting -O0." OFF)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'. This is the correct setting for normal builds.")
endif()

include(IrodsExternals)
include(IrodsCXXCompiler)

macro(IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_DEVELOP_DEPENDENCIES_LIST DEPENDENCY_NAME DEPENDENCY_SUBDIRECTORY)
  IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(${DEPENDENCY_NAME} ${DEPENDENCY_SUBDIRECTORY})
  list(APPEND IRODS_DEVELOP_DEPENDENCIES_LIST irods-externals-${DEPENDENCY_SUBDIRECTORY})
endmacro()

IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(ARCHIVE libarchive3.5.2-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(AVRO avro1.11.0-1)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(BOOST boost1.78.0-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(FMT fmt8.1.1-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(NANODBC nanodbc2.13.0-1)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(ZMQ zeromq4-14.1.8-0)

IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_DEVELOP_DEPENDENCIES_LIST(CATCH2 catch22.13.8-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_DEVELOP_DEPENDENCIES_LIST(CPPZMQ cppzmq4.8.1-1)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_DEVELOP_DEPENDENCIES_LIST(JSON json3.10.4-0)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_DEVELOP_DEPENDENCIES_LIST(SPDLOG spdlog1.9.2-1)

string(REPLACE ";" ", " IRODS_PACKAGE_DEPENDENCIES_STRING "${IRODS_PACKAGE_DEPENDENCIES_LIST}")
string(REPLACE ";" ", " IRODS_DEVELOP_DEPENDENCIES_STRING "${IRODS_DEVELOP_DEPENDENCIES_LIST}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(IRODS_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
# export-dynamic so stacktrace entries from executables have function names
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--export-dynamic -Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")

set(IRODS_VERSION_MAJOR "4")
set(IRODS_VERSION_MINOR "3")
set(IRODS_VERSION_PATCH "0")
set(IRODS_VERSION "${IRODS_VERSION_MAJOR}.${IRODS_VERSION_MINOR}.${IRODS_VERSION_PATCH}")
set(IRODS_PACKAGE_REVISION "1")

project(irods
  VERSION ${IRODS_VERSION}
  LANGUAGES C CXX)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(IRODS_BUILD_WITH_WERROR_DEFAULT ON)
else()
  set(IRODS_BUILD_WITH_WERROR_DEFAULT OFF)
endif()

set(IRODS_BUILD_WITH_WERROR ${IRODS_BUILD_WITH_WERROR_DEFAULT} CACHE BOOL "Choose whether to compile with -Werror.")

if (IRODS_BUILD_WITH_WERROR)
  add_compile_options(-Werror)
endif()

include(UseLibCXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/InstallDirs.cmake)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if (IRODS_DISABLE_COMPILER_OPTIMIZATIONS)
  add_compile_options(-O0)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)
endif()

add_compile_options(-Wall -Wextra)

check_cxx_compiler_flag(-Wno-write-strings can_use_wwrite_strings_cxx)
if (can_use_wwrite_strings_cxx)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-write-strings>)
endif()
check_c_compiler_flag(-Wno-unused-function can_use_wunused_function_c)
check_cxx_compiler_flag(-Wno-unused-function can_use_wunused_function_cxx)
if (can_use_wunused_function_c)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-unused-function>)
endif()
if (can_use_wunused_function_cxx)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-function>)
endif()
check_c_compiler_flag(-Wno-unused-parameter can_use_wunused_parameter_c)
check_cxx_compiler_flag(-Wno-unused-parameter can_use_wunused_parameter_cxx)
if (can_use_wunused_parameter_c)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>)
endif()
if (can_use_wunused_parameter_cxx)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>)
endif()
check_c_compiler_flag(-Wno-parentheses can_use_wparentheses_c)
check_cxx_compiler_flag(-Wno-parentheses can_use_wparentheses_cxx)
if (can_use_wparentheses_c)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-parentheses>)
endif()
if (can_use_wparentheses_cxx)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-parentheses>)
endif()
check_cxx_compiler_flag(-Wno-ignored-qualifiers can_use_wignored_qualifiers_cxx)
check_c_compiler_flag(-Wno-ignored-qualifiers can_use_wignored_qualifiers_c)
if (can_use_wignored_qualifiers_c)
  add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-ignored-qualifiers>)
endif()
if (can_use_wignored_qualifiers_cxx)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-ignored-qualifiers>)
endif()

if (IRODS_BUILD_WITH_WERROR)
  check_c_compiler_flag(-Wno-error=unused-variable can_use_wunused_variable_c)
  check_cxx_compiler_flag(-Wno-error=unused-variable can_use_wunused_variable_cxx)
  if (can_use_wunused_variable_c)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-error=unused-variable>)
  endif()
  if (can_use_wunused_variable_cxx)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-error=unused-variable>)
  endif()
  check_c_compiler_flag(-Wno-error=unused-but-set-variable can_use_wunused_but_set_variable_c)
  check_cxx_compiler_flag(-Wno-error=unused-but-set-variable can_use_wunused_but_set_variable_cxx)
  if (can_use_wunused_but_set_variable_c)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-error=unused-but-set-variable>)
  endif()
  if (can_use_wunused_but_set_variable_cxx)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-error=unused-but-set-variable>)
  endif()
  # Do not error on definition of implicit copy constructor for classes with a
  # user-declared copy assignment operator.
  # This is needed because the boost headers throw this warning.
  check_cxx_compiler_flag(-Wno-error=deprecated-copy can_use_wdeprecated_copy_cxx)
  if (can_use_wdeprecated_copy_cxx)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-error=deprecated-copy>)
  endif()
  # Do not error when overlapping comparisons always evaluate to false.
  # This is needed because the boost headers throw this warning.
  check_c_compiler_flag(-Wno-error=tautological-overlap-compare can_use_wtautological_overlap_compare_c)
  check_cxx_compiler_flag(-Wno-error=tautological-overlap-compare can_use_wtautological_overlap_compare_cxx)
  if (can_use_wtautological_overlap_compare_c)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-error=tautological-overlap-compare>)
  endif()
  if (can_use_wtautological_overlap_compare_cxx)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-error=tautological-overlap-compare>)
  endif()
endif()

find_package(nlohmann_json "3.6.1" REQUIRED)

include_directories(${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include)

if (NOT DEFINED THREADS_PREFER_PTHREAD_FLAG)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)
if (NOT PAM_LIBRARY)
  find_library(PAM_LIBRARY pam)
  if (PAM_LIBRARY)
    message(STATUS "Found pam: ${PAM_LIBRARY}")
  else()
    message(FATAL_ERROR "pam library not found")
  endif()
endif()

set(IRODS_PLATFORM_STRING linux_platform)
set(IRODS_COMPILE_DEFINITIONS ${IRODS_COMPILE_DEFINITIONS}
                              ${IRODS_PLATFORM_STRING}
                              _LARGEFILE_SOURCE
                              _FILE_OFFSET_BITS=64
                              _LARGE_FILES
                              _LARGEFILE64_SOURCE
                              BOOST_SYSTEM_NO_DEPRECATED
                              BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0
                              BOOST_STACKTRACE_USE_ADDR2LINE
                              BOOST_ASIO_NO_DEPRECATED
                              # Manually defined because Boost cannot detect that std::result_of
                              # has been removed in C++20. Verified that Boost 1.77 and Boost 1.78
                              # do not handle this yet. Normally, defining BOOST_ASIO_HAS_ macros
                              # should be avoided except when absolutely required as a workaround
                              # for the latest version of a compiler or platform.
                              #
                              # See the following for more information:
                              #
                              #   https://www.boost.org/doc/libs/1_77_0/doc/html/boost_asio/using.html
                              #
                              BOOST_ASIO_HAS_STD_INVOKE_RESULT
                              SPDLOG_FMT_EXTERNAL
                              SPDLOG_NO_TLS)

if (NOT IRODS_LINUX_DISTRIBUTION_NAME)
  execute_process(
    COMMAND "python3" "-c" "from __future__ import print_function; import distro; print(distro.id(), end='')"
    RESULT_VARIABLE IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_NAME
    OUTPUT_VARIABLE IRODS_LINUX_DISTRIBUTION_NAME
    )
  if (NOT ${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_NAME} STREQUAL "0")
    message(FATAL_ERROR "Linux platform name detection failed\n${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_NAME}")
  endif()
  set(IRODS_LINUX_DISTRIBUTION_NAME ${IRODS_LINUX_DISTRIBUTION_NAME} CACHE STRING "Linux distribution name, e.g. {ubuntu, centos, ...}." FORCE)
  message(STATUS "Setting unspecified IRODS_LINUX_DISTRIBUTION_NAME to '${IRODS_LINUX_DISTRIBUTION_NAME}'")
endif()

if (NOT IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR)
  execute_process(
    COMMAND "python3" "-c" "from __future__ import print_function; import distro; print(distro.major_version(), end='')"
    RESULT_VARIABLE IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_MAJOR
    OUTPUT_VARIABLE IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR
    )
  if (NOT ${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_MAJOR} STREQUAL "0")
    message(FATAL_ERROR "Linux platform name detection failed\n${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
  set(IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR} CACHE STRING "Linux distribution name, e.g. {ubuntu, centos, ...}." FORCE)
  message(STATUS "Setting unspecified IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR to '${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}'")
endif()

if(IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "debian")
  if (NOT IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
    execute_process(
      COMMAND "lsb_release" "-s" "-c"
      RESULT_VARIABLE IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME
      OUTPUT_VARIABLE IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME
      )
    string(STRIP ${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME} IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
    if (NOT ${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME} STREQUAL "0")
      message(FATAL_ERROR "Linux lsb_release shortname detection failed\n${IRODS_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME}")
    endif()
    set(IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME ${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME} CACHE STRING "Linux distribution version codename, e.g. {precise, wheezy, trusty, jessie, ...}." FORCE)
    message(STATUS "Setting unspecified IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME to '${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}'")
  endif()
endif()

if (NOT CPACK_GENERATOR)
  if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "arch")
    set(CPACK_GENERATOR TGZ CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
  elseif(IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "debian")
    set(CPACK_GENERATOR DEB CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
    message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
  else()
    set(IRODS_SUPPORTED_RPM_BASED_LINUX_DISTRIBUTION_NAMES "centos" "opensuse" "fedora" "almalinux" "rocky")
    if(IRODS_LINUX_DISTRIBUTION_NAME IN_LIST IRODS_SUPPORTED_RPM_BASED_LINUX_DISTRIBUTION_NAMES)
      set(CPACK_GENERATOR RPM CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
      message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
    else()
      message(FATAL_ERROR "Failed to set CPACK_GENERATOR for unknown Linux Distribution: ${IRODS_LINUX_DISTRIBUTION_NAME}")
    endif()
  endif()
endif()
set(IRODS_CPACK_GENERATOR ${CPACK_GENERATOR})

set(CPACK_PACKAGE_FILE_NAME "irods")

if (CPACK_GENERATOR STREQUAL "DEB")
  # CPACK_DEBIAN_PACKAGE_VERSION was previously a cache variable. Evict it.
  unset(CPACK_DEBIAN_PACKAGE_VERSION CACHE)
  set(CPACK_DEBIAN_PACKAGE_VERSION "${IRODS_VERSION}")
  set(CPACK_DEBIAN_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME)
    set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_DEBIAN_PACKAGE_RELEASE}~${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}")
  endif()
elseif (CPACK_GENERATOR STREQUAL "RPM")
  set(CPACK_RPM_PACKAGE_VERSION "${IRODS_VERSION}")
  set(CPACK_RPM_PACKAGE_RELEASE "${IRODS_PACKAGE_REVISION}")
endif()

# CPACK names component package files as "${CPACK_PACKAGE_FILE_NAME}-${COMPONENT_NAME}"
# CPackDeb TO_UPPERs all the component names when checking the ${CPACK_DEBIAN_<COMPONENT>...} variables
set(IRODS_PACKAGE_COMPONENT_SERVER_NAME "server")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_SERVER_NAME} IRODS_PACKAGE_COMPONENT_SERVER_NAME_UPPERCASE)
if (CPACK_GENERATOR STREQUAL "RPM")
  set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "devel")
  string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
elseif(CPACK_GENERATOR STREQUAL "DEB")
  set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "dev")
  string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
else()
  set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "dev")
  string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
endif()

string(TOUPPER ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_RUNTIME_NAME "runtime")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME} IRODS_PACKAGE_COMPONENT_RUNTIME_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_POSTGRES_NAME "database-plugin-postgres")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME} IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_MYSQL_NAME "database-plugin-mysql")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_MYSQL_NAME} IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE)
set(IRODS_PACKAGE_COMPONENT_ORACLE_NAME "database-plugin-oracle")
string(TOUPPER ${IRODS_PACKAGE_COMPONENT_ORACLE_NAME} IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json")
  message(FATAL_ERROR "Missing file: ${CMAKE_CURRENT_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
Are your git submodules initialized? If not, try running the following in the source directory:
git submodule update --init")
endif()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/core/include/server_control_plane_command.hpp
  COMMAND LD_LIBRARY_PATH=${IRODS_EXTERNALS_FULLPATH_BOOST}/lib ${IRODS_EXTERNALS_FULLPATH_AVRO}/bin/avrogencpp -n irods -o ${CMAKE_CURRENT_BINARY_DIR}/lib/core/include/server_control_plane_command.hpp -i ${CMAKE_CURRENT_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/irods_schema_messaging/v1/server_control_plane_command.json
  )

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_server_control_plane.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib/core/include/server_control_plane_command.hpp
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include/irods_default_paths.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/lib/core/include/irods_default_paths.hpp
  @ONLY
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include/rodsVersion.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/lib/core/include/rodsVersion.h
  @ONLY
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include/irods_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/lib/core/include/irods_version.h
  @ONLY
  )

include(GetGitRevisionDescription)
get_git_head_revision(IRODS_GIT_REFSPEC IRODS_GIT_SHA1)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.json.dist.in
  ${CMAKE_CURRENT_BINARY_DIR}/VERSION.json.dist
  @ONLY
  )

add_library(
  irods_filesystem_path
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/path.cpp
  )
target_include_directories(
  irods_filesystem_path
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  )
target_compile_definitions(irods_filesystem_path PRIVATE ${IRODS_COMPILE_DEFINITIONS})
set_target_properties(irods_filesystem_path PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_filesystem_path PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(
  irods_filesystem_client
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/filesystem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/collection_iterator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/recursive_collection_iterator.cpp
  )
target_link_libraries(
  irods_filesystem_client
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_filesystem_client
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/icat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/re/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_filesystem_client PRIVATE ${IRODS_COMPILE_DEFINITIONS})
set_target_properties(irods_filesystem_client PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_filesystem_client PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(
  irods_filesystem_server
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/filesystem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/collection_iterator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/src/recursive_collection_iterator.cpp
  )
target_link_libraries(
  irods_filesystem_server
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_filesystem_server
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/icat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/re/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_filesystem_server PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API IRODS_TRANSPORT_ENABLE_SERVER_SIDE_API)
set_target_properties(irods_filesystem_server PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_filesystem_server PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(
  irods_user_administration_client
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/user_administration.cpp
  )
target_link_libraries(
  irods_user_administration_client
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_user_administration_client
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_user_administration_client PRIVATE ${IRODS_COMPILE_DEFINITIONS})
set_target_properties(irods_user_administration_client PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_user_administration_client PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


add_library(
  irods_user_administration_server
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/user_administration.cpp
  )
target_link_libraries(
  irods_user_administration_server
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_user_administration_server
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/api/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_user_administration_server PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API)
set_target_properties(irods_user_administration_server PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_user_administration_server PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(
  irods_resource_administration_client
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/resource_administration.cpp
  )
target_link_libraries(
  irods_resource_administration_client
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_resource_administration_client
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_resource_administration_client PRIVATE ${IRODS_COMPILE_DEFINITIONS})
set_target_properties(irods_resource_administration_client PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_resource_administration_client PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(
  irods_resource_administration_server
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/resource_administration.cpp
  )
target_link_libraries(
  irods_resource_administration_server
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_resource_administration_server
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/api/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_resource_administration_server PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API)
set_target_properties(irods_resource_administration_server PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_resource_administration_server PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# This list of source files exist because they must be compiled multiple times.
# Once for the irods_client target and again for the irods_server target. The reason
# for this is due to the use of macros such as ENABLE_RE and the irods::plugin_base class
# being shared between client-side and server-side code.
set(
  IRODS_LIBIRODS_CLIENT_API_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcAuthCheck.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcAuthPluginRequest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcAuthRequest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcAuthResponse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcBulkDataObjPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcBulkDataObjReg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcChkNVPathPerm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcChkObjPermAndStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcClientHints.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcCloseCollection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcCollCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcCollRepl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataCopy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjChksum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjCopy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjCreateAndStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjLock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjLseek.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjOpen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjOpenAndStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjPhymv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjRename.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjRepl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjRsync.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjTrim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjTruncate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjUnlink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataObjWrite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcDataPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcEndTransaction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcExecCmd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcExecMyRule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcExecRuleExpression.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileChksum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileChmod.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileClosedir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileGetFsFreeSpace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileLseek.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileMkdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileOpen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileOpendir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFilePut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileReaddir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileRename.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileRmdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileStageToCache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileSyncToArch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileTruncate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileUnlink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcFileWrite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGenQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGeneralAdmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGeneralRowInsert.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGeneralRowPurge.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGeneralUpdate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetHierFromLeafId.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetHierarchyForResc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetHostForGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetHostForPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetLimitedPassword.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetMiscSvrInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetRemoteZoneResc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetRescQuota.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetTempPassword.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcGetTempPasswordForOther.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcIESClientHints.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcL3FileGetSingleBuf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcL3FilePutSingleBuf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcModAVUMetadata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcModAccessControl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcModColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcModDataObjMeta.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcObjStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcOpenCollection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcOprComplete.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcPamAuthRequest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcPhyBundleColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcPhyPathReg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcProcStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcQuerySpecColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcReadCollection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRegColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRegDataObj.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRegReplica.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRmColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRuleExecDel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRuleExecMod.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcRuleExecSubmit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcServerReport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSimpleQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSpecificQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSslEnd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSslStart.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcStreamClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcStreamRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcStructFileBundle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcStructFileExtAndReg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcStructFileExtract.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcStructFileSync.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileClosedir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileLseek.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileMkdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileOpen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileOpendir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFilePut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileReaddir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileRename.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileRmdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileTruncate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileUnlink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSubStructFileWrite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcSyncMountedColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcTicketAdmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcUnbunAndRegPhyBunfile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcUnregDataObj.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcUserAdmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rcZoneReport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_atomic_apply_acl_operations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_atomic_apply_metadata_operations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_data_object_finalize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_data_object_modify_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_get_file_descriptor_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_register_physical_path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_replica_close.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_replica_open.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/api/src/rc_touch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/bunUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/chksumUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/clientLogin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/client_connection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/connection_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/cpUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/fsckUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/getUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_c_api.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_client_api_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_client_negotiation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/lsUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/mcollUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/miscUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/mkdirUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/mvUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/phybunUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/phymvUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/procApiRequest.cpp # relies on RODS_SERVER/CLERVER
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/putUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcConnect.cpp # relies on RODS_CLERVER
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcPortalOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/regUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/replUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rmUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rmdirUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rmtrashUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rsyncUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/scanUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/sockComm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/sslSockComm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/trimUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/user.cpp
  )
add_library(
  # This object file exists to avoid recompiling the API functions multiple
  # times. It is used by the irods_client and RodsAPIs target. These libraries
  # use the same build arguments.
  irods_client_api_functions
  OBJECT
  ${IRODS_LIBIRODS_CLIENT_API_SOURCES}
  )
target_link_libraries(
  irods_client_api_functions
  PUBLIC
  nlohmann_json::nlohmann_json
  OpenSSL::Crypto
  OpenSSL::SSL
  )
target_include_directories(
  irods_client_api_functions
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/icat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/re/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_client_api_functions PRIVATE ${IRODS_COMPILE_DEFINITIONS})
set_target_properties(irods_client_api_functions PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_client_api_functions PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

set(
  IRODS_LIBIRODS_CLIENT_SOURCES
  $<TARGET_OBJECTS:irods_client_api_functions>
  $<TARGET_OBJECTS:irods_filesystem_path>
  $<TARGET_OBJECTS:irods_filesystem_client>
  $<TARGET_OBJECTS:irods_user_administration_client>
  $<TARGET_OBJECTS:irods_resource_administration_client>
  )
add_library(
  irods_client
  SHARED
  ${IRODS_LIBIRODS_CLIENT_SOURCES}
  )
target_link_libraries(
  irods_client
  PUBLIC
  irods_plugin_dependencies
  irods_common
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  OpenSSL::SSL
  OpenSSL::Crypto
  ${CMAKE_DL_LIBS}
  )
target_include_directories(
  irods_client
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/api/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
set_property(TARGET irods_client PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_client PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_client PRIVATE ${IRODS_COMPILE_DEFINITIONS})

set(
  IRODS_LIBIRODS_SERVER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_atomic_apply_acl_operations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_atomic_apply_metadata_operations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_data_object_finalize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_get_file_descriptor_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_register_physical_path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_replica_close.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_replica_open.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rs_touch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsAuthCheck.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsAuthPluginRequest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsAuthRequest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsAuthResponse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsBulkDataObjPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsBulkDataObjReg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsChkNVPathPerm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsChkObjPermAndStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsClientHints.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsCloseCollection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsCollCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsCollRepl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataCopy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjChksum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjCopy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjCreateAndStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjLock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjLseek.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjOpen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjOpenAndStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjPhymv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjRename.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjRepl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjRsync.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjTrim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjTruncate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjUnlink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataObjWrite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsDataPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsEndTransaction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsExecCmd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsExecMyRule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsExecRuleExpression.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileChksum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileChmod.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileClosedir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileGetFsFreeSpace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileLseek.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileMkdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileOpen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileOpendir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFilePut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileReaddir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileRename.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileRmdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileStageToCache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileSyncToArch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileTruncate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileUnlink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsFileWrite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGenQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGeneralAdmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGeneralRowInsert.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGeneralRowPurge.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGeneralUpdate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetHierFromLeafId.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetHierarchyForResc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetHostForGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetHostForPut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetLimitedPassword.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetMiscSvrInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetRemoteZoneResc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetRescQuota.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetTempPassword.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsGetTempPasswordForOther.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsIESClientHints.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsL3FileGetSingleBuf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsL3FilePutSingleBuf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsModAVUMetadata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsModAccessControl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsModColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsModDataObjMeta.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsObjStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsOpenCollection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsOprComplete.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsPamAuthRequest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsPhyBundleColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsPhyPathReg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsProcStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsQuerySpecColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsReadCollection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRegColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRegDataObj.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRegReplica.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRmColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRuleExecDel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRuleExecMod.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsRuleExecSubmit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsServerReport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSimpleQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSpecificQuery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSslEnd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSslStart.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsStreamClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsStreamRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsStructFileBundle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsStructFileExtAndReg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsStructFileExtract.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsStructFileSync.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileClose.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileClosedir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileCreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileGet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileLseek.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileMkdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileOpen.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileOpendir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFilePut.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileRead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileReaddir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileRename.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileRmdir.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileStat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileTruncate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileUnlink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSubStructFileWrite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsSyncMountedColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsTicketAdmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsUnbunAndRegPhyBunfile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsUnregDataObj.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsUserAdmin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/api/src/rsZoneReport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/administration_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/atomic_apply_database_operations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/client_api_whitelist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/catalog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/catalog_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/collection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/dataObjOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/replica_access_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/replica_state_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/fileOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/finalize_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/initServer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_api_calling_functions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_api_number_validator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_collection_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_data_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_database_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_database_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_database_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_database_plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_file_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_get_l1desc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_report_plugins_in_json.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_mysql_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_oracle_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_physical_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_postgres_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_generic_database_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_resource_backport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_resource_constants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_resource_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_resource_plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_resource_plugin_impostor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_resource_redirect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_rs_comm_query.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_server_api_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_server_globals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_server_negotiation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_server_state.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_signal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_structured_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/json_deserialization.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/json_serialization.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/logical_locking.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/miscServerFunct.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/objDesc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/objMetaOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/physPath.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/plugin_lifetime_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/procLog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/replication_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/rodsAgent.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/rodsConnect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/rsApiHandler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/rsIcatOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/rsLog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/server_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/specColl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/user_validation_utilities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/voting.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/src/fileDriver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/icat/src/icatHighLevelRoutines.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/extractAvuMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/genQueryMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/icatAdminMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/icatGeneralMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/irods_ms_plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/irods_re_plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/irods_re_structs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/reAction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/nre.systemMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/irods_re_namespaceshelper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/irods_re_ruleexistshelper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/irods_re_serialization.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/keyValPairMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/locks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/mailMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/miscMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/printMS.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/reDataObjOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/reIn2p3SysRule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/reNaraMetaData.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/reStruct.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/reSysDataObjOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/sharedmemory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/re/src/testMS.cpp
  $<TARGET_OBJECTS:irods_filesystem_server>
  $<TARGET_OBJECTS:irods_user_administration_server>
  $<TARGET_OBJECTS:irods_resource_administration_server>
  )
add_library(
  irods_server
  SHARED
  ${IRODS_LIBIRODS_SERVER_SOURCES}

  # The client-side APIs must be recompiled due to macros like ENABLE_RE.
  # This allows the PEPs to fire when operations are invoked via irods::plugin_base::call.
  ${IRODS_LIBIRODS_CLIENT_API_SOURCES}
  $<TARGET_OBJECTS:irods_filesystem_path>
  $<TARGET_OBJECTS:irods_filesystem_client>
  $<TARGET_OBJECTS:irods_user_administration_client>
  $<TARGET_OBJECTS:irods_resource_administration_client>
  )
target_link_libraries(
  irods_server
  PUBLIC
  nlohmann_json::nlohmann_json
  irods_common
  irods_plugin_dependencies
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  ${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  OpenSSL::SSL
  OpenSSL::Crypto
  ${CMAKE_DL_LIBS}
  rt
  )
target_include_directories(
  irods_server
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/api/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  ${IRODS_EXTERNALS_FULLPATH_NANODBC}/include
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
  )
set_property(TARGET irods_server PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_server PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_server PRIVATE ENABLE_RE RODS_CLERVER IRODS_ENABLE_SYSLOG ${IRODS_COMPILE_DEFINITIONS})


set(
  IRODS_LIBIRODS_COMMON_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/base64.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/dns_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/getRodsEnv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/hashtable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/hostname_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_buffer_encryption.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_children_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_configuration_keywords.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_configuration_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_default_paths.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_environment_properties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_error.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_exception.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_get_full_path_for_config_file.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_hierarchy_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_hostname.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_kvp_string_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_pack_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_parse_command_line_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_path_recursion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_pluggable_auth_scheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_plugin_name_generator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_random.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_serialization.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_server_properties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_socket_information.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_stacktrace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_string_tokenize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_virtual_path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/key_value_proxy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/msParam.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/obf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/osauth.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/packStruct.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/parseCommandLine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcGlobal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcMisc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/region.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rodsError.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rodsLog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rodsPath.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/stringOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/version.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/Hasher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/MD5Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/SHA256Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/SHA512Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/ADLER32Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/SHA1Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/checksum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/irods_hasher_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpBase_c.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpReceiver_c.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpSender_c.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_logger.cpp
  )
add_library(
  irods_common
  SHARED
  ${IRODS_LIBIRODS_COMMON_SOURCES}
  $<TARGET_OBJECTS:irods_filesystem_path>
  )
target_link_libraries(
  irods_common
  PUBLIC
  nlohmann_json::nlohmann_json
  Threads::Threads
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_random.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  OpenSSL::Crypto
  OpenSSL::SSL
  ${CMAKE_DL_LIBS}
  rt
  CURL::libcurl
  )
target_include_directories(
  irods_common
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/icat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/re/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
set_property(TARGET irods_common PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_common PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_common PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_ENABLE_SYSLOG)

add_library(
  irods_plugin_dependencies_functions
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/apiHandler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_auth_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_auth_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_auth_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_generic_auth_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_gsi_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_krb_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_native_auth_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_network_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_network_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_network_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_osauth_auth_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_pam_auth_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_ssl_object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_tcp_object.cpp
  )
target_link_libraries(
  irods_plugin_dependencies_functions
  PUBLIC
  nlohmann_json::nlohmann_json
  )
target_include_directories(
  irods_plugin_dependencies_functions
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/icat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/re/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_plugin_dependencies_functions PRIVATE ${IRODS_COMPILE_DEFINITIONS})
set_target_properties(irods_plugin_dependencies_functions PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_plugin_dependencies_functions PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(
  irods_plugin_dependencies
  SHARED
  $<TARGET_OBJECTS:irods_plugin_dependencies_functions>
  )
target_link_libraries(
  irods_plugin_dependencies
  PUBLIC
  irods_common
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_random.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  ${CMAKE_DL_LIBS}
  )
target_include_directories(
  irods_plugin_dependencies
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  )
set_property(TARGET irods_plugin_dependencies PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_plugin_dependencies PROPERTY SOVERSION ${IRODS_VERSION})
target_compile_definitions(irods_plugin_dependencies PRIVATE ${IRODS_COMPILE_DEFINITIONS})

set(
  IRODS_LIB_CORE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/base64.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/dns_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/getRodsEnv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/hashtable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/hostname_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_buffer_encryption.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_children_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_configuration_keywords.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_configuration_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_default_paths.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_environment_properties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_error.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_exception.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_get_full_path_for_config_file.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_hierarchy_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_kvp_string_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_pack_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_parse_command_line_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_path_recursion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_pluggable_auth_scheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_plugin_name_generator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_random.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_serialization.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_server_properties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_socket_information.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_stacktrace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_string_tokenize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/irods_virtual_path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/msParam.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/obf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/osauth.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/packStruct.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/parseCommandLine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcGlobal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcMisc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rcPortalOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/region.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rodsError.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rodsLog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/rodsPath.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/stringOpr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/src/version.cpp
  )

set(
  IRODS_LIB_HASHER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/Hasher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/MD5Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/SHA256Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/SHA512Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/ADLER32Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/SHA1Strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/checksum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/src/irods_hasher_factory.cpp
  )

set(
  IRODS_LIB_RBUDP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpBase_c.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpReceiver_c.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/src/QUANTAnet_rbudpSender_c.cpp
  )

set(
  IRODS_LIB_CLIENT_EXEC_SOURCES
  )

set(
  IRODS_SERVER_CORE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_logger.cpp
  )

add_library(
  RodsAPIs
  STATIC
  ${IRODS_LIB_CORE_SOURCES}
  ${IRODS_LIB_RBUDP_SOURCES}
  ${IRODS_LIB_HASHER_SOURCES}
  ${IRODS_LIB_CLIENT_EXEC_SOURCES}
  ${IRODS_SERVER_CORE_SOURCES}
  $<TARGET_OBJECTS:irods_client_api_functions>
  $<TARGET_OBJECTS:irods_filesystem_path>
  $<TARGET_OBJECTS:irods_filesystem_client>
  $<TARGET_OBJECTS:irods_filesystem_server>
  $<TARGET_OBJECTS:irods_user_administration_client>
  $<TARGET_OBJECTS:irods_user_administration_server>
  $<TARGET_OBJECTS:irods_resource_administration_client>
  $<TARGET_OBJECTS:irods_resource_administration_server>
  $<TARGET_OBJECTS:irods_plugin_dependencies_functions>
  )
target_link_libraries(
  RodsAPIs
  PUBLIC
  nlohmann_json::nlohmann_json
  OpenSSL::Crypto
  OpenSSL::SSL
  CURL::libcurl
  Threads::Threads
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  ${CMAKE_DL_LIBS}
  rt
  )
target_include_directories(
  RodsAPIs
  PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irods>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/hasher/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/rbudp/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/core/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/icat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/re/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/drivers/include>
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(RodsAPIs PRIVATE ${IRODS_COMPILE_DEFINITIONS})

set(
  IRODS_MAIN_EXECUTABLE_IRODSSERVER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/rodsServer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_server_control_plane.cpp
  )

set(
  IRODS_MAIN_EXECUTABLE_HOSTNAME_RESOLVES_TO_LOCAL_ADDRESS_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/hostname_resolves_to_local_address.cpp
  )

set(
  IRODS_MAIN_EXECUTABLES
  irodsServer
  hostname_resolves_to_local_address
  )

foreach(EXECUTABLE ${IRODS_MAIN_EXECUTABLES})
  string(TOUPPER ${EXECUTABLE} EXECUTABLE_UPPERCASE)
  add_executable(
    ${EXECUTABLE}
    ${IRODS_MAIN_EXECUTABLE_${EXECUTABLE_UPPERCASE}_SOURCES}
    )
  target_link_libraries(
    ${EXECUTABLE}
    PRIVATE
    irods_common
    irods_plugin_dependencies
    irods_server
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
    rt
    ${CMAKE_DL_LIBS}
    m
    )
  target_include_directories(
    ${EXECUTABLE}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
    )
  target_compile_definitions(
    ${EXECUTABLE}
    PRIVATE
    ENABLE_RE
    ${IRODS_COMPILE_DEFINITIONS}
    IRODS_ENABLE_SYSLOG
    )
endforeach()

add_executable(
  irodsReServer
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irodsReServer.cpp
  )
target_link_libraries(
  irodsReServer
  PRIVATE
  irods_server
  irods_client
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_container.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  ${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so
  rt
  ${CMAKE_DL_LIBS}
  m
  )
target_include_directories(
  irodsReServer
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  ${IRODS_EXTERNALS_FULLPATH_NANODBC}/include
  )
target_compile_definitions(irodsReServer PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_ENABLE_SYSLOG)

add_executable(
  irods_api_test_harness
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_api_test_harness.cpp
  )
target_link_libraries(
  irods_api_test_harness
  PRIVATE
  irods_server
  irods_client
  irods_plugin_dependencies
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  rt
  ${CMAKE_DL_LIBS}
  m
  )
target_include_directories(
  irods_api_test_harness
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_api_test_harness PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_ENABLE_SYSLOG)

add_executable(
  irods_configuration_test_harness
  ${CMAKE_CURRENT_SOURCE_DIR}/server/core/src/irods_configuration_test_harness.cpp
  )
target_link_libraries(
  irods_configuration_test_harness
  PRIVATE
  irods_server
  irods_client
  irods_plugin_dependencies
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  rt
  ${CMAKE_DL_LIBS}
  m
  )
target_include_directories(
  irods_configuration_test_harness
  PRIVATE
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
  )
target_compile_definitions(irods_configuration_test_harness PRIVATE ${IRODS_COMPILE_DEFINITIONS} IRODS_ENABLE_SYSLOG)

add_executable(
  irodsPamAuthCheck
  ${CMAKE_CURRENT_SOURCE_DIR}/server/auth/src/irodsPamAuthCheck.cpp
  )
target_link_libraries(
  irodsPamAuthCheck
  PRIVATE
  ${PAM_LIBRARY}
  )

add_executable(
  genOSAuth
  ${CMAKE_CURRENT_SOURCE_DIR}/server/auth/src/genOSAuth.cpp
  )
target_link_libraries(
  genOSAuth
  PRIVATE
  irods_client
  irods_plugin_dependencies
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
  ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
  )
target_compile_definitions(genOSAuth PRIVATE ${IRODS_COMPILE_DEFINITIONS})

add_subdirectory(plugins/api)
add_subdirectory(plugins/auth)
add_subdirectory(plugins/database)
add_subdirectory(plugins/microservices)
add_subdirectory(plugins/network)
add_subdirectory(plugins/resources)
add_subdirectory(plugins/rule_engines)
add_subdirectory(plugins/experimental)
add_subdirectory(test/c_api_test)
add_subdirectory(test/post_install_test)
add_subdirectory(unit_tests)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/development_library.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/runtime_library.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/server.cmake)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_PACKAGE_VERSION ${IRODS_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${IRODS_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${IRODS_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${IRODS_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System")
set(CPACK_PACKAGE_VENDOR "Renaissance Computing Institute <info@irods.org>")

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_VENDOR "iRODS Consortium")
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/sbin")
set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "rocky")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/lib")
endif()

### TODO better place for these
## database plugins
# postgres
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_NAME "irods-database-plugin-postgres")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}), libodbc1, odbcinst, odbc-postgresql, postgresql-client, super, libc6")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_CONFLICTS "irods-database-plugin-mysql, irods-database-plugin-oracle")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME_UPPERCASE}_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_NAME "irods-database-plugin-postgres")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6" OR IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "7")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, unixODBC, postgresql, authd, postgresql-odbc")
  else()
    message(FATAL "Unsupported CentOS major version: ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "almalinux" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "rocky")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, unixODBC, postgresql, authd, postgresql-odbc")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, unixODBC, postgresql, psqlODBC")
endif()
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PACKAGE_CONFLICTS "irods-database-plugin-mysql, irods-database-plugin-oracle")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/preinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_POSTGRES_NAME}_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postrm")

# mysql
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_NAME "irods-database-plugin-mysql")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu")
  set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}), libodbc1, odbcinst, mysql-client, libc6")
endif()
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_CONFLICTS "irods-database-plugin-postgres, irods-database-plugin-oracle")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME_UPPERCASE}_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_NAME "irods-database-plugin-mysql")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6" OR IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "7")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, mysql, unixODBC, mysql-connector-odbc")
  else()
    message(FATAL "Unsupported CentOS major version: ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "almalinux")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, mysql, unixODBC, mysql-connector-odbc")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "rocky")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, mysql, unixODBC")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, mysql-client, unixODBC, MyODBC-unixODBC")
endif()
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PACKAGE_CONFLICTS "irods-database-plugin-postgres, irods-database-plugin-oracle")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/preinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postrm")

# oracle
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_NAME "irods-database-plugin-oracle")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server (= ${CPACK_DEBIAN_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}), libodbc1, odbcinst, libc6")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_CONFLICTS "irods-database-plugin-mysql, irods-database-plugin-postgres")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME_UPPERCASE}_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/preinst;${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postinst;${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postrm;")

set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_NAME "irods-database-plugin-oracle")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  if (IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "6" OR IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR STREQUAL "7")
    set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, unixODBC")
  else()
    message(FATAL "Unsupported CentOS major version: ${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}")
  endif()
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "almalinux" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "rocky" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-server = ${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}, unixODBC")
endif()
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PACKAGE_CONFLICTS "irods-database-plugin-mysql, irods-database-plugin-postgres")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/preinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postinst")
set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_ORACLE_NAME}_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/plugins/database/packaging/postrm")

# Install configuration schemas with updated id field
set(IRODS_CONFIGURATION_SCHEMA_VERSION 4)
set(
  IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}
  )
set(
  IRODS_CONFIGURATION_SCHEMA_FILES
  VERSION.json
  client_environment.json
  client_hints.json
  configuration_directory.json
  database_config.json
  grid_status.json
  host_access_control.json
  host_resolution.json
  plugin.json
  resource.json
  rule_engine.json
  server.json
  server_config.json
  server_status.json
  service_account_environment.json
  zone_bundle.json
  )
foreach(SCHEMA_FILE ${IRODS_CONFIGURATION_SCHEMA_FILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}/${SCHEMA_FILE}
    ${IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY}/${SCHEMA_FILE}
    COPYONLY
    )
  install(
    FILES
    ${IRODS_CONFIGURATION_SCHEMA_BUILD_DIRECTORY}/${SCHEMA_FILE}
    DESTINATION ${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
    )
endforeach()
install(
  CODE
  "execute_process(
    COMMAND
    python3 \"${CMAKE_CURRENT_SOURCE_DIR}/configuration_schemas/update_schema_ids_for_cmake.py\" \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}\" \"\${CMAKE_INSTALL_PREFIX}/${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}\"
  )"
  COMPONENT
  ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/IRODSConfig.cmake.not_yet_installed.in
  ${CMAKE_CURRENT_BINARY_DIR}/IRODSConfig.cmake.not_yet_installed # suffix prevents cmake's find_package() from using this copy of the file
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/irods/cmake
  PATH_VARS IRODS_INCLUDE_DIRS
  )

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/IRODSConfig.cmake.not_yet_installed
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/irods/cmake
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
  RENAME IRODSConfig.cmake
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/IRODSConfigVersion.cmake
  VERSION ${IRODS_VERSION}
  COMPATIBILITY ExactVersion
  )

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/IRODSConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/irods/cmake
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
  )

install(
  FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/RequireOutOfSourceBuild.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/GetGitRevisionDescription.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/GetGitRevisionDescription.cmake.in
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/CheckCPPMacroDefinition.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/CheckCPPMacroDefinition.c.in
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/FindLibCXX.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/UseLibCXX.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/Findnlohmann_json.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/IrodsExternals.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/IrodsCXXCompiler.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/irods/cmake/Modules
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
  )

foreach(DATABASE_PLUGIN postgres oracle mysql)
  string(TOUPPER ${DATABASE_PLUGIN} DATABASE_PLUGIN_UPPER)
  add_custom_target(
    non-package-install-${DATABASE_PLUGIN}
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_SERVER_NAME} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake"
    COMMAND
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${IRODS_PACKAGE_COMPONENT_${DATABASE_PLUGIN_UPPER}_NAME} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake"
    DEPENDS
    irodsServer irodsReServer irods_api_test_harness irods_configuration_test_harness hostname_resolves_to_local_address irodsPamAuthCheck irods_client irods_server irods_common irods_plugin_dependencies RodsAPIs helloworld_server helloworld_client msisync_to_archive msi_update_unixfilesystem_resource_free_space compound deferred load_balanced mockarchive nonblocking passthru random replication structfile univmss unixfilesystem irods_rule_engine_plugin-irods_rule_language irods_rule_engine_plugin-cpp_default_policy irods_rule_engine_plugin-passthrough native_client native_server osauth_client osauth_server pam_client pam_server ssl_client ssl_server tcp_client tcp_server genOSAuth mytest
    ${DATABASE_PLUGIN} IRODS_PHONY_TARGET_icatSysTables_${DATABASE_PLUGIN}.sql
    )
endforeach()

include(CPack)
