set(
  IRODS_PLUGINS_RESOURCES_REPLICATION_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_create_write_replicator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_object_oper.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_repl_rebalance.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_repl_retry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_replicator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/librepl.cpp"
)

set(
  IRODS_PLUGINS_RESOURCES_REPLICATION_HEADERS_PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_create_write_replicator.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_object_oper.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_oper_replicator.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_repl_rebalance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_repl_retry.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_repl_types.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/private/irods_replicator.hpp"
)

add_library(
  irods_resource_plugin-replication
  MODULE
  "${IRODS_PLUGINS_RESOURCES_REPLICATION_SOURCES}"
  "${IRODS_PLUGINS_RESOURCES_REPLICATION_HEADERS_PRIVATE}"
)
set_property(TARGET irods_resource_plugin-replication PROPERTY LIBRARY_OUTPUT_NAME "replication")
target_link_libraries(
  irods_resource_plugin-replication
  PRIVATE
  irods_common
  irods_server
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  fmt::fmt
  OpenSSL::Crypto
)
target_include_directories(
  irods_resource_plugin-replication
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
)
target_compile_definitions(
  irods_resource_plugin-replication
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
)

add_dependencies(all-plugins-resources irods_resource_plugin-replication)
install(
  TARGETS
  irods_resource_plugin-replication
  LIBRARY
  DESTINATION "${IRODS_PLUGINS_DIRECTORY}/resources"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
