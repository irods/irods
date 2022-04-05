cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

# create phony target to build everything in server folder
add_custom_target(all-server)

# create irods_server target first, as it's needed in executables
add_library(
  irods_server
  SHARED
)

add_subdirectory(core)
add_subdirectory(api)
add_subdirectory(icat)
add_subdirectory(re)
add_subdirectory(drivers)
add_subdirectory(auth)
add_subdirectory(control_plane)
add_subdirectory(main_server)
add_subdirectory(hostname_resolves_to_local_address)
add_subdirectory(delay_server)
add_subdirectory(irods_grid)
add_subdirectory(harness)

target_link_objects(
  irods_server
  PUBLIC
  irods_lib_api_server
  irods_lib_core_server
  irods_server_api
  irods_server_core
  irods_filesystem_path
  irods_filesystem_client
  irods_filesystem_server
  irods_user_administration_client
  irods_user_administration_server
  irods_resource_administration_client
  irods_resource_administration_server
  irods_server_drivers
  irods_server_icat
  irods_server_re
)
target_link_libraries(
  irods_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so"
  OpenSSL::SSL
  OpenSSL::Crypto
  ${CMAKE_DL_LIBS}
  rt
)
target_include_directories(
  irods_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/include"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/include"
)
set_property(TARGET irods_server PROPERTY VERSION ${IRODS_VERSION})
set_property(TARGET irods_server PROPERTY SOVERSION ${IRODS_VERSION})

install(
  TARGETS
  irods_server
  EXPORT IRODSTargets
  LIBRARY
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
)

# Install configuration schemas with updated id field
set(IRODS_CONFIGURATION_SCHEMA_VERSION 4)
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
  unattended_installation.json
  zone_bundle.json
)
foreach(SCHEMA_FILE IN LISTS IRODS_CONFIGURATION_SCHEMA_FILES)
  configure_file(
    "${CMAKE_IRODS_SOURCE_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}/${SCHEMA_FILE}.in"
    "${CMAKE_IRODS_BINARY_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}/${SCHEMA_FILE}"
    @ONLY
  )
  install(
    FILES
    "${CMAKE_IRODS_BINARY_DIR}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}/${SCHEMA_FILE}"
    DESTINATION "${IRODS_HOME_DIRECTORY}/configuration_schemas/v${IRODS_CONFIGURATION_SCHEMA_VERSION}"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
endforeach()

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/irods"
  DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/init.d"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

install(
  DIRECTORY
  DESTINATION "${IRODS_HOME_DIRECTORY}/config/lockFileDir"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  DIRECTORY
  DESTINATION "${IRODS_HOME_DIRECTORY}/config/packedRei"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES "${CMAKE_IRODS_BINARY_DIR}/VERSION.json.dist"
  DESTINATION "${IRODS_HOME_DIRECTORY}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  DIRECTORY
  DESTINATION "${IRODS_HOME_DIRECTORY}/log"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  DIRECTORY
  DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/README.md"
  DESTINATION "${IRODS_DOC_DIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  RENAME readme
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/LICENSE"
  DESTINATION "${IRODS_DOC_DIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/connectControl.config.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/core.dvm.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/core.fnm.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/core.re.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/database_config.json.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/host_access_control_config.json.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/hosts_config.json.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/irodsMonPerf.config.in"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/irods.logrotate"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/irods.rsyslog"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/server_config.json.template"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/server_log_setup_instructions.txt"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/server_setup_instructions.txt"
  DESTINATION "${IRODS_HOME_DIRECTORY}/packaging"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/find_os.sh"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/postinstall.sh"
  "${CMAKE_IRODS_SOURCE_DIR}/packaging/preremove.sh"
  DESTINATION "${IRODS_HOME_DIRECTORY}/packaging"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  DIRECTORY "${CMAKE_IRODS_SOURCE_DIR}/scripts"
  DESTINATION "${IRODS_HOME_DIRECTORY}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/msiExecCmd_bin/irodsServerMonPerf"
  "${CMAKE_IRODS_SOURCE_DIR}/msiExecCmd_bin/test_execstream.py"
  "${CMAKE_IRODS_SOURCE_DIR}/msiExecCmd_bin/hello"
  DESTINATION "${IRODS_HOME_DIRECTORY}/msiExecCmd_bin"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/msiExecCmd_bin/univMSSInterface.sh.template"
  DESTINATION "${IRODS_HOME_DIRECTORY}/msiExecCmd_bin"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/test/test_framework_configuration.json"
  DESTINATION "${IRODS_HOME_DIRECTORY}/test"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  DIRECTORY "${CMAKE_IRODS_SOURCE_DIR}/test/filesystem"
  DESTINATION "${IRODS_HOME_DIRECTORY}/test"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES
  "${CMAKE_IRODS_SOURCE_DIR}/irodsctl"
  DESTINATION "${IRODS_HOME_DIRECTORY}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
)
