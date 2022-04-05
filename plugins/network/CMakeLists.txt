cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

# create phony target to build everything in plugins/network folder
add_custom_target(all-plugins-network)
add_dependencies(all-plugins_no_database all-plugins-network)

set(
  IRODS_NETWORK_PLUGINS
  ssl
  tcp
)

foreach(plugin IN LISTS IRODS_NETWORK_PLUGINS)
  set(plugin_target_part "irods_network_plugin-${plugin}")
  foreach(variant IN ITEMS client server)
    set(plugin_target "${plugin_target_part}_${variant}")
    set(plugin_name "${plugin}_${variant}")

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
      irods_plugin_dependencies
      irods_${variant}
      "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
      "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
      "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
      OpenSSL::SSL
      OpenSSL::Crypto
    )
    target_include_directories(
      ${plugin_target}
      PRIVATE
      "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
      "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
      "${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include"
    )
    target_compile_definitions(
      ${plugin_target}
      PRIVATE
      ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    )

    add_dependencies(all-plugins-network ${plugin_target})
    install(
      TARGETS
      ${plugin_target}
      LIBRARY
      DESTINATION "${IRODS_PLUGINS_DIRECTORY}/network"
      COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
    )
  endforeach()

  target_compile_definitions(
    ${plugin_target_part}_server
    PRIVATE
    ENABLE_RE
    IRODS_ENABLE_SYSLOG
  )
endforeach()
