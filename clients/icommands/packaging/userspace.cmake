if(NOT ICOMMANDS_USERSPACE_TARBALL_PLATFORM_NAME)
  if(IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME AND NOT IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME STREQUAL "none")
    set(ICOMMANDS_USERSPACE_TARBALL_PLATFORM_NAME "${IRODS_LINUX_DISTRIBUTION_VERSION_CODENAME}")
  elseif(IRODS_LINUX_DISTRIBUTION_NAME)
    if(IRODS_LINUX_DISTRIBUTION_VERSION AND NOT IRODS_LINUX_DISTRIBUTION_VERSION STREQUAL "none")
      set(ICOMMANDS_USERSPACE_TARBALL_PLATFORM_NAME "${IRODS_LINUX_DISTRIBUTION_NAME}${IRODS_LINUX_DISTRIBUTION_VERSION}")
    else()
      set(ICOMMANDS_USERSPACE_TARBALL_PLATFORM_NAME "${IRODS_LINUX_DISTRIBUTION_NAME}")
    endif()
  else()
    set(ICOMMANDS_USERSPACE_TARBALL_PLATFORM_NAME "${CMAKE_SYSTEM_NAME}")
  endif()
endif()

set(ICOMMANDS_USERSPACE_TARBALL_FILENAME
    "irods-icommands-${icommands_VERSION}-${ICOMMANDS_USERSPACE_TARBALL_PLATFORM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}.tar.gz"
    CACHE STRING
    "Filename of userspace tarball package to generate")
set(ICOMMANDS_USERSPACE_TARBALL_ROOT_DIRNAME
    "irods-icommands-${icommands_VERSION}"
    CACHE STRING
    "Name of userspace tarball package's root directory. Set to empty string to use the root of the tarball as the root directory.")

set(ICOMMANDS_USERSPACE_TARBALL_PATH "${CMAKE_CURRENT_BINARY_DIR}/${ICOMMANDS_USERSPACE_TARBALL_FILENAME}")

# fetch the parts of irods-runtime currently exposed
set(irods_runtime_components "irods_client;irods_common;irods_plugin_dependencies")
foreach(irods_runtime_component ${irods_runtime_components})
  unset(iimported_location)
  get_target_property(iimported_location ${irods_runtime_component} LOCATION)
  list(APPEND IRODS_RUNTIME_LIBRARY_PATHS "${iimported_location}")
  unset(iimported_location)
endforeach()

file(
  GLOB ICOMMANDS_USERSPACE_TARBALL_INSTRUMENTATION
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/packaging/userspace/*"
  )

add_custom_command(
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/packaging/userspace/userspace-packager.py" -v
    "--cmake-path=${CMAKE_COMMAND}"
    "$<$<BOOL:${CMAKE_OBJDUMP}>:--objdump-path=${CMAKE_OBJDUMP}>"
    "$<$<BOOL:${CMAKE_READELF}>:--readelf-path=${CMAKE_READELF}>"
    "$<$<BOOL:${CMAKE_STRIP}>:--strip-path=${CMAKE_STRIP}>"
    "$<$<BOOL:${IRODS_LINUX_DISTRIBUTION_NAME}>:--target-platform=${IRODS_LINUX_DISTRIBUTION_NAME}>"
    "$<$<BOOL:${IRODS_LINUX_DISTRIBUTION_VERSION}>:--target-platform-variant=${IRODS_LINUX_DISTRIBUTION_VERSION}>"
    "--cmake-install-prefix=${CMAKE_INSTALL_PREFIX}"
    "--cmake-install-bindir=${CMAKE_INSTALL_BINDIR}"
    "--cmake-install-sbindir=${CMAKE_INSTALL_SBINDIR}"
    "--cmake-install-libdir=${CMAKE_INSTALL_LIBDIR}"
    "--irods-package-prefix=${IRODS_PACKAGE_PREFIX}"
    "--irods-install-prefix=${IRODS_INSTALL_PREFIX}"
    "--irods-install-bindir=${IRODS_INSTALL_BINDIR}"
    "--irods-install-sbindir=${IRODS_INSTALL_SBINDIR}"
    "--irods-install-libdir=${IRODS_INSTALL_LIBDIR}"
    "--irods-home-dir=${IRODS_HOME_DIRECTORY}"
    "--irods-pluginsdir=${IRODS_PLUGINS_DIRECTORY}"
    "$<$<BOOL:${CMAKE_SHARED_LIBRARY_SUFFIX}>:--sharedlib-suffix=${CMAKE_SHARED_LIBRARY_SUFFIX}>"
    "$<$<BOOL:${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES}>:--sharedlib-suffix=$<JOIN:${CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES},;--sharedlib-suffix=>>"
    "$<$<BOOL:${IRODS_RUNTIME_LIBRARY_PATHS}>:--irods-runtime-lib=$<JOIN:${IRODS_RUNTIME_LIBRARY_PATHS},;--irods-runtime-lib=>>"
    "$<$<BOOL:${LIBCXX_LIBRARIES}>:--libcxx-libpath=$<JOIN:${LIBCXX_LIBRARIES},;--libcxx-libpath=>>"
    "--build-dir=${CMAKE_CURRENT_BINARY_DIR}"
    "--work-dir=${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/userspace-tarball"
    "--cmake-tmpdir=${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp"
    "--install-component=icommands"
    "$<$<BOOL:${IRODS_CLIENT_ICOMMANDS_SCRIPTS}>:--script-icommand=$<JOIN:${IRODS_CLIENT_ICOMMANDS_SCRIPTS},;--script-icommand=>>"
    "$<$<BOOL:${ICOMMANDS_USERSPACE_TARBALL_ROOT_DIRNAME}>:--tarball-root-dir-name=${ICOMMANDS_USERSPACE_TARBALL_ROOT_DIRNAME}>"
    "--output=${ICOMMANDS_USERSPACE_TARBALL_PATH}"
  DEPENDS icommands "${ICOMMANDS_USERSPACE_TARBALL_INSTRUMENTATION}"
  OUTPUT "${ICOMMANDS_USERSPACE_TARBALL_PATH}"
  JOB_POOL console
  COMMAND_EXPAND_LISTS
  )
add_custom_target(userspace-tarball DEPENDS "${ICOMMANDS_USERSPACE_TARBALL_PATH}")
