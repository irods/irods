cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_library(
  irods_resource_plugin-replication
  MODULE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_create_write_replicator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_object_oper.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_repl_rebalance.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_repl_retry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_replicator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/librepl.cpp"
)
set_property(TARGET irods_resource_plugin-replication PROPERTY LIBRARY_OUTPUT_NAME "replication")
target_link_libraries(
  irods_resource_plugin-replication
  PRIVATE
  irods_common
  irods_server
  "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
  OpenSSL::Crypto
)
target_include_directories(
  irods_resource_plugin-replication
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
  "${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include"
)
target_compile_definitions(
  irods_resource_plugin-replication
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
)

install(
  TARGETS
  irods_resource_plugin-replication
  LIBRARY
  DESTINATION "${IRODS_PLUGINS_DIRECTORY}/resources"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
