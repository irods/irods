# create phony target to build everything in plugins/rule_engines folder
add_custom_target(all-plugins-rule_engines)
add_dependencies(all-plugins_no_database all-plugins-rule_engines)

add_subdirectory(irods_rule_language)

set(
  IRODS_RULE_ENGINE_PLUGINS
  irods_rule_language
  cpp_default_policy
  passthrough
)

set(
  IRODS_PLUGINS_RE_HEADERS_PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/arithmetics.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/cache.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/cache.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/cache.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/configuration.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/conversion.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/datetime.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/debug.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/end.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/filesystem.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/functions.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/index.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/key.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/key.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/parser.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reFuncDefs.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reGlobals.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reGlobalsExtern.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reHelpers1.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reVariableMap.gen.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reVariableMap.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/reVariables.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/region.check.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/region.check.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/region.to.region.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/region.to.region.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/region.to.region2.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/region.to.region2.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/restruct.templates.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/restructs.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/rules.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/to.memory.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/to.memory.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/to.region.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/to.region.proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/traversal.instance.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/typing.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include/irods/private/re/utils.hpp"
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
    "${IRODS_PLUGINS_RE_HEADERS_PRIVATE}"
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
    fmt::fmt
    ${CMAKE_DL_LIBS}
  )
  target_include_directories(
    ${plugin_target}
    PRIVATE
    #"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/irods_rule_language/include>"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  )
  target_compile_definitions(
    ${plugin_target}
    PRIVATE
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    ENABLE_RE
    IRODS_ENABLE_SYSLOG
  )

  add_dependencies(all-plugins-rule_engines ${plugin_target})
  install(
    TARGETS
    ${plugin_target}
    LIBRARY
    DESTINATION "${IRODS_PLUGINS_DIRECTORY}/rule_engines"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
endforeach()
