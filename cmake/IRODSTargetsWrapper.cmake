# We can't define targets in our config file because the variables it defines
# and the modules it adds to the search path need to be available before any
# languages are enabled.
# This is because we bundle a module to set the compilers to the clang we
# provide via externals, and downstream projects which use this module must
# include it before languages are enabled.
# Therefore, we do not include our targets file in our CMake configuration
# file, we have a wrapper for our targets file that pulls in our dependencies,
# and we define a variable to make including our targets file
# easy for downstream projects.
# Yes, this is icky. We plan to move away from this model eventually.

include(CMakeFindDependencyMacro)

if (NOT DEFINED THREADS_PREFER_PTHREAD_FLAG)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
endif()
find_dependency(Threads)

find_dependency(nlohmann_json "3.6.1")
find_dependency(OpenSSL COMPONENTS Crypto SSL)
find_dependency(fmt "8.1.1")
find_dependency(spdlog "1.9.2")

if (TARGET fmt::fmt-header-only AND NOT TARGET fmt::fmt)
  add_library(fmt::fmt ALIAS fmt::fmt-header-only)
endif()
if (TARGET spdlog::spdlog_header_only AND NOT TARGET spdlog::spdlog)
  add_library(spdlog::spdlog ALIAS spdlog::spdlog_header_only)
endif()

include("${IRODS_TARGETS_PATH_UNWRAPPED}")

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0" AND NOT IRODS_BUILD_AGAINST_LIBCXX)
  message(AUTHOR_WARNING "Adding `consteval=constexpr` to irods_common's `INTERFACE_COMPILE_DEFINITIONS` to work around Clang bug with libstdc++ 13.")
  target_compile_definitions(irods_common INTERFACE "consteval=constexpr")
endif()
