cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

# create phony target to build everything in plugins/experimental folder
add_custom_target(all-plugins-experimental)
add_dependencies(all-plugins_no_database all-plugins-experimental)

set(
  IRODS_EXPERIMENTAL_API_PLUGINS
  copy
  remove
  replicate
)

foreach(plugin IN LISTS IRODS_EXPERIMENTAL_API_PLUGINS)
  string(TOUPPER ${plugin} PLUGIN_UPPERCASE)
  set(plugin_target "irods_experimental_plugin-${plugin}")
  set(plugin_name "${plugin}")

  add_library(
    ${plugin_target}
    MODULE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${plugin}.cpp"
  )
  set_property(TARGET ${plugin_target} PROPERTY LIBRARY_OUTPUT_NAME ${plugin_name})
  target_link_libraries(
    ${plugin_target}
    PRIVATE
    irods_server
    irods_common
    "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
    fmt::fmt
    ${CMAKE_DL_LIBS}
  )
  target_include_directories(
    ${plugin_target}
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  )
  target_compile_definitions(
    ${plugin_target}
    PRIVATE
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    ENABLE_RE
    IRODS_ENABLE_SYSLOG
  )

  add_dependencies(all-plugins-experimental ${plugin_target})
  install(
    TARGETS
    ${plugin_target}
    LIBRARY
    DESTINATION "${IRODS_PLUGINS_DIRECTORY}/experimental"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
endforeach()
