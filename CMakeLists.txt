cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets
# CURL::libcurl target
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
include(RequireOutOfSourceBuild)

option(IRODS_DISABLE_COMPILER_OPTIMIZATIONS "Disables compiler optimizations by setting -O0." OFF)
option(IRODS_ENABLE_ADDRESS_SANITIZER "Enables detection of memory leaks and other features provided by Address Sanitizer." OFF)
option(IRODS_ENABLE_ALL_TESTS "Enables all tests (e.g. C/C++ unit tests, microservices specifically designed for testing)." OFF)

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
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(AVRO avro1.11.0-2)
IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(BOOST boost1.81.0-0)
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
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--export-dynamic -Wl,--enable-new-dtags -Wl,--as-needed")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-Wl,--enable-new-dtags -Wl,--as-needed")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,--enable-new-dtags -Wl,--as-needed")

if (IRODS_ENABLE_ADDRESS_SANITIZER)
  # The following options are compiled into all binaries and allow them to start even
  # when an ODR violation is detected.
  #
  # Make sure the correct llvm-symbolizer binary is available to Address Sanitizer. This binary
  # allows debug symbols to be reported appropriately. There are two ways to do this:
  #
  #     export PATH=/opt/irods-externals/clang13.0.0-0/bin:$PATH
  #
  # - or -
  #
  #     export ASAN_SYMBOLIZER_PATH=/opt/irods-externals/clang13.0.0-0/bin/llvm-symbolizer
  #
  add_compile_definitions(IRODS_ADDRESS_SANITIZER_DEFAULT_OPTIONS="log_path=/tmp/irods_asan_output:detect_odr_violation=1")
  add_compile_options(
    -fsanitize=address
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -O1
  )
  add_link_options(
    -fsanitize=address
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -O1
  )
else()
  set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,-z,defs")
  set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_MODULE_LINKER_FLAGS_INIT} -Wl,-z,defs")
  set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,-z,defs")
endif()

set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-Wl,--gc-sections -Wl,-z,combreloc")
include(IrodsRunpathDefaults)

set(IRODS_VERSION_MAJOR "4")
set(IRODS_VERSION_MINOR "3")
set(IRODS_VERSION_PATCH "2")
set(IRODS_VERSION "${IRODS_VERSION_MAJOR}.${IRODS_VERSION_MINOR}.${IRODS_VERSION_PATCH}")
set(IRODS_PACKAGE_REVISION "0")

project(irods
  VERSION ${IRODS_VERSION}
  LANGUAGES C CXX)

include(ObjectTargetHelpers)
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
find_package(fmt "8.1.1" REQUIRED
  HINTS "${IRODS_EXTERNALS_FULLPATH_FMT}")
find_package(spdlog "1.9.2" REQUIRED
  HINTS "${IRODS_EXTERNALS_FULLPATH_SPDLOG}")


if (NOT DEFINED THREADS_PREFER_PTHREAD_FLAG)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)

include(IrodsCPackPlatform)

set(IRODS_PLATFORM_STRING linux_platform)

set(
  IRODS_COMPILE_DEFINITIONS_PUBLIC
  ${IRODS_PLATFORM_STRING}
  _LARGEFILE_SOURCE
  _FILE_OFFSET_BITS=64
  _LARGE_FILES
  _LARGEFILE64_SOURCE
)
set(
  IRODS_COMPILE_DEFINITIONS_PRIVATE
  BOOST_SYSTEM_NO_DEPRECATED
  BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0
  BOOST_STACKTRACE_USE_ADDR2LINE
  BOOST_ASIO_NO_DEPRECATED
  # Historically, iRODS has used OpenSSL API version 1.1.x. OpenSSL 3.x deprecated and even removed some API calls
  # in use by iRODS. This requires a compile definition which specifies the API version with which the target
  # should be compatible. For more information, see https://www.openssl.org/docs/man3.0/man7/OPENSSL_API_COMPAT.html
  OPENSSL_API_COMPAT=10100
  SPDLOG_FMT_EXTERNAL
  SPDLOG_NO_TLS
)

# Define our component names here so they're defined everywhere
set(IRODS_PACKAGE_COMPONENT_RUNTIME_NAME "irods-runtime")
set(IRODS_PACKAGE_COMPONENT_SERVER_NAME "irods-server")
set(IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME "irods-dev")
set(IRODS_PACKAGE_COMPONENT_POSTGRES_NAME "irods-database-plugin-postgres")
set(IRODS_PACKAGE_COMPONENT_MYSQL_NAME "irods-database-plugin-mysql")
set(IRODS_PACKAGE_COMPONENT_ORACLE_NAME "irods-database-plugin-oracle")

set(CMAKE_IRODS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_IRODS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

include(GetGitRevisionDescription)
get_git_head_revision(IRODS_GIT_REFSPEC IRODS_GIT_SHA1)
configure_file(
  "${CMAKE_IRODS_SOURCE_DIR}/version.json.dist.in"
  "${CMAKE_IRODS_BINARY_DIR}/version.json.dist"
  @ONLY
)

add_subdirectory(lib)
add_subdirectory(server)
add_subdirectory(plugins)
add_subdirectory(test/c_api_test)
add_subdirectory(test/post_install_test)
add_subdirectory(unit_tests)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/packaging.cmake")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/IRODSConfig.cmake.not_yet_installed.in"
  "${CMAKE_IRODS_BINARY_DIR}/IRODSConfig.cmake.not_yet_installed" # suffix prevents cmake's find_package() from using this copy of the file
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/irods/cmake"
  PATH_VARS IRODS_INCLUDE_DIRS
)
configure_file(
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/IRODSConfigVersion.cmake.in"
  "${CMAKE_IRODS_BINARY_DIR}/IRODSConfigVersion.cmake"
  @ONLY
)

install(
  FILES
  "${CMAKE_IRODS_BINARY_DIR}/IRODSConfig.cmake.not_yet_installed"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/irods/cmake"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
  RENAME IRODSConfig.cmake
)

install(
  FILES
  "${CMAKE_IRODS_BINARY_DIR}/IRODSConfigVersion.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IRODSTargetsWrapper.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/irods/cmake"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/RequireOutOfSourceBuild.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/GetGitRevisionDescription.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/GetGitRevisionDescription.cmake.in"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/CheckCPPMacroDefinition.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/CheckCPPMacroDefinition.c.in"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/FindLibCXX.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/UseLibCXX.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/Findnlohmann_json.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/IrodsExternals.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/IrodsCXXCompiler.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/ObjectTargetHelpers.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/IrodsCPackPlatform.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/IrodsCPackCommon.cmake"
  "${CMAKE_IRODS_SOURCE_DIR}/cmake/Modules/IrodsRunpathDefaults.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/irods/cmake/Modules"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)

install(
  EXPORT
  IRODSTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/irods/cmake"
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
  )
  add_dependencies(
    non-package-install-${DATABASE_PLUGIN}
    all-lib
    all-server
    all-plugins_no_database
    irods_database_plugin-${DATABASE_PLUGIN}
    IRODS_PHONY_TARGET_icatSysTables_${DATABASE_PLUGIN}.sql
    irodsTestPutGet
  )
  if (IRODS_UNIT_TESTS_BUILD)
    add_dependencies(
      non-package-install-${DATABASE_PLUGIN}
      all-unit_tests
    )
  endif()
endforeach()

include(CPack)
