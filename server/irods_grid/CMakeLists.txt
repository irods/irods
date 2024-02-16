# irods-grid was previously part of the iCommands repo

cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_executable(
  irods-grid
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods-grid.cpp"
)
target_link_libraries(
  irods-grid
  PRIVATE
  irods_common
  irods_plugin_dependencies
  irods_client
  irods_server_control_plane
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so"
  ${CMAKE_DL_LIBS}
  m
)
target_include_directories(
  irods-grid
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/include"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/include"
)
target_compile_definitions(
  irods-grid
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
)

add_dependencies(all-server irods-grid)
install(
  TARGETS
  irods-grid
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
