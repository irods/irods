cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_subdirectory(irods_rule_language)

set(
  IRODS_RULE_ENGINE_PLUGINS
  irods_rule_language
  cpp_default_policy
  passthrough
)

foreach(plugin IN LISTS IRODS_RULE_ENGINE_PLUGINS)
  set(plugin_target "irods_rule_engine_plugin-${plugin}")
  set(plugin_name "${plugin_target}")

  if (TARGET ${plugin_target})
    continue()
  endif()

  add_library(
    ${plugin_target}
    MODULE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${plugin}.cpp"
  )
  #set_property(TARGET ${plugin_target} PROPERTY LIBRARY_OUTPUT_NAME ${plugin_name})
  target_link_libraries(
    ${plugin_target}
    PRIVATE
    irods_server
    irods_common
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
    "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
    ${CMAKE_DL_LIBS}
  )
  target_include_directories(
    ${plugin_target}
    PRIVATE
    #"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include>"
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

  install(
    TARGETS
    ${plugin_target}
    LIBRARY
    DESTINATION "${IRODS_PLUGINS_DIRECTORY}/rule_engines"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
endforeach()
