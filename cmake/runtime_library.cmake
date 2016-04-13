install(
  TARGETS
  irods_clerver
  irods_client_api
  irods_client_api_table
  irods_client_core
  irods_client_plugins
  irods_common
  irods_plugin_dependencies
  irods_server
  EXPORT IRODSTargets
  LIBRARY
  DESTINATION usr/lib
  COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
  )

set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME_UPPERCASE}_PACKAGE_NAME "irods-runtime")
set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME_UPPERCASE}_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, libc6, sudo, libssl1.0.0, libfuse2, python, openssl, python-psutil, python-requests")


set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}_PACKAGE_NAME "irods-runtime")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, openssl, python, python-psutil, python-requests, python-jsonschema")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, libopenssl1_0_0, python, openssl, python-psutil, python-requests, python-jsonschema")
endif()
