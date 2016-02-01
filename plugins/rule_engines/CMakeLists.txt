set(
  IRODS_RULE_ENGINE_PLUGIN_RE_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re/libre.cpp
  )

set(
  IRODS_RULE_ENGINE_PLUGIN_RE-AUDIT_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-audit/libre-audit.cpp
  )

set(
  IRODS_RULE_ENGINE_PLUGIN_RE-IRODS_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/libre-irods.cpp
  )

set(
  IRODS_RULE_ENGINE_PLUGINS
  re
  re-audit
  re-irods
  )

foreach(PLUGIN ${IRODS_RULE_ENGINE_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_library(
    ${PLUGIN}
    MODULE
    ${IRODS_RULE_ENGINE_PLUGIN_${PLUGIN_UPPERCASE}_SOURCES}
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
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so
    )

  target_compile_definitions(${PLUGIN} PRIVATE RODS_SERVER ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_RULE_ENGINE_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_HOME_DIRECTORY}/plugins/re
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
