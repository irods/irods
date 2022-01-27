set(
  IRODS_AUTH_PLUGIN_native_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/native/libnative.cpp
  )

set(
  IRODS_AUTH_PLUGIN_osauth_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/osauth/libosauth.cpp
  )

set(
  IRODS_AUTH_PLUGIN_pam_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/pam/libpam.cpp
  )

set(
  IRODS_AUTH_PLUGIN_irods_auth_plugin-native_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/irods_auth_plugin-native/libirods_auth_plugin-native.cpp
  )

set(
  IRODS_AUTH_PLUGINS
  # original auth plugins - these are required to exist to talk to older servers
  native
  osauth
  pam
  # auth plugins using 4.3.0 framework
  irods_auth_plugin-native
  )

set(
  IRODS_AUTH_PLUGIN_COMPILE_DEFINITIONS_client
  )

set(
  IRODS_AUTH_PLUGIN_COMPILE_DEFINITIONS_server
  RODS_SERVER
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
  )

set(
  IRODS_AUTH_PLUGIN_LINK_LIBRARIES_client
  irods_client
  )

set(
  IRODS_AUTH_PLUGIN_LINK_LIBRARIES_server
  irods_server
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
  )

set(
  IRODS_AUTH_PLUGIN_TYPES
  client
  server
  )

foreach(PLUGIN ${IRODS_AUTH_PLUGINS})
  foreach(TYPE ${IRODS_AUTH_PLUGIN_TYPES})
    set(IRODS_CURRENT_PLUGIN_TARGET ${PLUGIN}_${TYPE})

    add_library(
      ${IRODS_CURRENT_PLUGIN_TARGET}
      MODULE
      ${IRODS_AUTH_PLUGIN_${PLUGIN}_SOURCES}
      )

    target_include_directories(
      ${IRODS_CURRENT_PLUGIN_TARGET}
      PRIVATE
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/server/api/include>
      ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
      ${IRODS_EXTERNALS_FULLPATH_FMT}/include
      )

    target_link_libraries(
      ${IRODS_CURRENT_PLUGIN_TARGET}
      PRIVATE
      ${IRODS_AUTH_PLUGIN_LINK_LIBRARIES_${TYPE}}
      irods_common
      irods_plugin_dependencies
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
      ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
      ${CMAKE_DL_LIBS}
      OpenSSL::Crypto
      nlohmann_json::nlohmann_json
      )

    target_compile_definitions(${IRODS_CURRENT_PLUGIN_TARGET} PRIVATE ${IRODS_AUTH_PLUGIN_COMPILE_DEFINITIONS_${TYPE}} ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
    target_compile_options(${IRODS_CURRENT_PLUGIN_TARGET} PRIVATE -Wno-write-strings)
    install(
      TARGETS
      ${IRODS_CURRENT_PLUGIN_TARGET}
      LIBRARY
      DESTINATION ${IRODS_PLUGINS_DIRECTORY}/auth
      COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
      )
    endforeach()
endforeach()
