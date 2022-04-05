cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

# create phony target to build everything in plugins/resources folder
add_custom_target(all-plugins-resources)
add_dependencies(all-plugins_no_database all-plugins-resources)

add_subdirectory(replication)

set(
  IRODS_RESOURCE_PLUGINS
  compound
  deferred
  load_balanced
  mockarchive
  nonblocking
  passthru
  random
  replication
  structfile
  univmss
  unixfilesystem
)

foreach(plugin IN LISTS IRODS_RESOURCE_PLUGINS)
  set(plugin_target "irods_resource_plugin-${plugin}")
  set(plugin_name "${plugin}")

  if (TARGET ${plugin_target})
    continue()
  endif()

  add_library(
    ${plugin_target}
    MODULE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${plugin}.cpp"
  )
  set_property(TARGET ${plugin_target} PROPERTY LIBRARY_OUTPUT_NAME ${plugin_name})
  target_link_libraries(
    ${plugin_target}
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
    ${plugin_target}
    PRIVATE
    "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
    "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
    "${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include"
  )
  target_compile_definitions(
    ${plugin_target}
    PRIVATE
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    ENABLE_RE
    IRODS_ENABLE_SYSLOG
  )

  add_dependencies(all-plugins-resources ${plugin_target})
  install(
    TARGETS
    ${plugin_target}
    LIBRARY
    DESTINATION "${IRODS_PLUGINS_DIRECTORY}/resources"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
endforeach()
