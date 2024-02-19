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
find_dependency(fmt "8.1.1"
  HINTS "${IRODS_EXTERNALS_FULLPATH_FMT}")
find_dependency(spdlog "1.9.2"
  HINTS "${IRODS_EXTERNALS_FULLPATH_SPDLOG}")

include("${IRODS_TARGETS_PATH_UNWRAPPED}")
