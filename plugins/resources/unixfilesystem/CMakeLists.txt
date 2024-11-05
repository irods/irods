# anything being unit tested needs to be split out into an object library.
add_library(
  irods_resource_plugin_obj-unixfilesystem
  OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_is_in_host_list.cpp"
)
objects_link_libraries(
  irods_resource_plugin_obj-unixfilesystem
  PUBLIC
  irods_common
  fmt::fmt
  Boost::headers
)
target_include_directories(
  irods_resource_plugin_obj-unixfilesystem
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_compile_definitions(
  irods_resource_plugin_obj-unixfilesystem
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG
)

add_library(
  irods_resource_plugin-unixfilesystem
  MODULE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/unixfilesystem.cpp"
)
set_property(TARGET irods_resource_plugin-unixfilesystem PROPERTY LIBRARY_OUTPUT_NAME "unixfilesystem")
target_link_objects(
  irods_resource_plugin-unixfilesystem
  PRIVATE
  irods_resource_plugin_obj-unixfilesystem
)
target_link_libraries(
  irods_resource_plugin-unixfilesystem
  PRIVATE
  irods_common
  irods_server
  "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so"
  Boost::filesystem
  Boost::system
  fmt::fmt
  OpenSSL::Crypto
)
target_include_directories(
  irods_resource_plugin-unixfilesystem
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include"
)
target_compile_definitions(
  irods_resource_plugin-unixfilesystem
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
)

add_dependencies(all-plugins-resources irods_resource_plugin-unixfilesystem)
install(
  TARGETS
  irods_resource_plugin-unixfilesystem
  LIBRARY
  DESTINATION "${IRODS_PLUGINS_DIRECTORY}/resources"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
