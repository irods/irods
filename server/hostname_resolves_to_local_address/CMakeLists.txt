add_executable(
  hostname_resolves_to_local_address
  "${CMAKE_CURRENT_SOURCE_DIR}/src/hostname_resolves_to_local_address.cpp"
)
target_link_libraries(
  hostname_resolves_to_local_address
  PRIVATE
  irods_common
  irods_plugin_dependencies
  irods_server
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so"
  rt
  ${CMAKE_DL_LIBS}
  m
)
target_include_directories(
  hostname_resolves_to_local_address
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/include"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/include"
)
target_compile_definitions(
  hostname_resolves_to_local_address
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
)

add_dependencies(all-server hostname_resolves_to_local_address)
install(
  TARGETS
  hostname_resolves_to_local_address
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
