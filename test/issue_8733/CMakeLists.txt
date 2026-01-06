add_executable(
  irods_test_issue_8733
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
target_link_libraries(
  irods_test_issue_8733
  PRIVATE
  irods_common
  irods_client
  irods_plugin_dependencies
)
target_include_directories(
  irods_test_issue_8733
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
)
target_compile_definitions(
  irods_test_issue_8733
  PRIVATE
  "${IRODS_COMPILE_DEFINITIONS_PUBLIC}"
  "${IRODS_COMPILE_DEFINITIONS_PRIVATE}"
)

install(
  TARGETS
  irods_test_issue_8733
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
