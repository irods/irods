install(
  TARGETS
    irods_clerver
    irods_client_api
    irods_client_api_table
    irods_client_core
    irods_client_plugins
    irods_server
  LIBRARY
    DESTINATION lib
    COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
  )

set(CPACK_DEBIAN_${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}_PACKAGE_NAME "irods-runtime")

## CPACK_DEBIAN_<COMPONENT>_PACKAGE_DEPENDS # TODO add irods-externals deps here
