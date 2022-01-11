find_package(OpenSSL REQUIRED)

set(
  IRODS_NETWORK_PLUGIN_ssl_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/network/ssl/libssl.cpp
  )

set(
  IRODS_NETWORK_PLUGIN_tcp_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/network/tcp/libtcp.cpp
  )

set(
  IRODS_NETWORK_PLUGINS
  ssl
  tcp
  )

set(
  IRODS_NETWORK_COMPILE_DEFINITIONS_client
  )

set(
  IRODS_NETWORK_COMPILE_DEFINITIONS_server
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
  )

set(
  IRODS_NETWORK_LINK_LIBRARIES_client
  irods_client
  )

set(
  IRODS_NETWORK_LINK_LIBRARIES_server
  irods_server
  )

set(
  IRODS_NETWORK_PLUGIN_TYPES
  client
  server
  )

foreach(PLUGIN ${IRODS_NETWORK_PLUGINS})
  foreach(TYPE ${IRODS_NETWORK_PLUGIN_TYPES})
    set(IRODS_CURRENT_PLUGIN_TARGET ${PLUGIN}_${TYPE})

    add_library(
      ${IRODS_CURRENT_PLUGIN_TARGET}
      MODULE
      ${IRODS_NETWORK_PLUGIN_${PLUGIN}_SOURCES}
      )

    target_include_directories(
      ${IRODS_CURRENT_PLUGIN_TARGET}
      PRIVATE
      ${CMAKE_BINARY_DIR}/lib/core/include
      ${CMAKE_SOURCE_DIR}/lib/core/include
      ${CMAKE_SOURCE_DIR}/lib/api/include
      ${CMAKE_SOURCE_DIR}/server/drivers/include
      ${CMAKE_SOURCE_DIR}/server/core/include
      ${CMAKE_SOURCE_DIR}/server/icat/include
      ${CMAKE_SOURCE_DIR}/server/re/include
      ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
      ${IRODS_EXTERNALS_FULLPATH_FMT}/include
      )

    target_link_libraries(
      ${IRODS_CURRENT_PLUGIN_TARGET}
      PRIVATE
      ${IRODS_NETWORK_LINK_LIBRARIES_${TYPE}}
      irods_plugin_dependencies
      irods_common
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
      OpenSSL::SSL
      OpenSSL::Crypto
      )

    target_compile_definitions(${IRODS_CURRENT_PLUGIN_TARGET} PRIVATE ${IRODS_NETWORK_COMPILE_DEFINTIONS_${TYPE}} ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
    target_compile_options(${IRODS_CURRENT_PLUGIN_TARGET} PRIVATE -Wno-write-strings)
    install(
      TARGETS
      ${IRODS_CURRENT_PLUGIN_TARGET}
      LIBRARY
      DESTINATION ${IRODS_PLUGINS_DIRECTORY}/network
      COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
      )
  endforeach()
endforeach()
