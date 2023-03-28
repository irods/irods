set(
  IRODS_MICROSERVICE_PLUGINS
  json_compare
  json_contains
  json_dump
  json_free
  json_handles
  json_names
  json_parse
  json_size
  json_type
  json_value
)

foreach (plugin IN LISTS IRODS_MICROSERVICE_PLUGINS)
  set(plugin_name "msi_${plugin}")
  
  add_library(
    ${plugin_name}
    MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}.cpp)
  
  target_compile_definitions(
    ${plugin_name}
    PRIVATE
    ENABLE_RE
    ${IRODS_COMPILE_DEFINITIONS}
    IRODS_ENABLE_SYSLOG)
  
  target_include_directories(
    ${plugin_name}
    PRIVATE
    ${CMAKE_BINARY_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/api/include
    ${CMAKE_SOURCE_DIR}/server/drivers/include
    ${CMAKE_SOURCE_DIR}/server/api/include
    ${CMAKE_SOURCE_DIR}/server/core/include
    ${CMAKE_SOURCE_DIR}/server/icat/include
    ${CMAKE_SOURCE_DIR}/server/re/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include)
  
  target_link_libraries(
    ${plugin_name}
    PRIVATE
    irods_server
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
  
  install(
    TARGETS ${plugin_name}
    LIBRARY DESTINATION ${IRODS_PLUGINS_DIRECTORY}/microservices
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME})
endforeach()
