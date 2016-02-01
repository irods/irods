find_package(OpenSSL REQUIRED)

set(
  IRODS_AUTH_PLUGIN_NATIVE_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/auth/native/libnative.cpp
  )

set(
  IRODS_AUTH_PLUGIN_OSAUTH_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/auth/osauth/libosauth.cpp
  )

set(
  IRODS_AUTH_PLUGIN_PAM_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/auth/pam/libpam.cpp
  )

set(
  IRODS_AUTH_PLUGINS
  native
  osauth
  pam
  )

foreach(PLUGIN ${IRODS_AUTH_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_library(
    ${PLUGIN}
    MODULE
    ${IRODS_AUTH_PLUGIN_${PLUGIN_UPPERCASE}_SOURCES}
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
    ${OPENSSL_INCLUDE_DIR}
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_clerver
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${OPENSSL_CRYPTO_LIBRARY}
    )

  target_compile_definitions(${PLUGIN} PRIVATE RODS_SERVER ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_AUTH_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_HOME_DIRECTORY}/plugins/auth
    COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
  )
