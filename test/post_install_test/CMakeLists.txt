add_executable(
  irodsTestPutGet
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
target_link_libraries(
  irodsTestPutGet
  PRIVATE
  irods_common
  irods_client
  irods_plugin_dependencies
)
target_include_directories(
  irodsTestPutGet
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
)

install(
  TARGETS
  irodsTestPutGet
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
