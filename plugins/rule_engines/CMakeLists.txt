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
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/arithmetics.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/cache.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/configuration.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/conversion.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/datetime.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/filesystem.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/msiHelper.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/index.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/nre.reHelpers1.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/nre.reHelpers2.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/nre.reLib1.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/parser.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/reStruct.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/reVariableMap.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/reVariableMap.gen.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/restructs.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/rules.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/typing.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/utils.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/rsRe.cpp
  ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/functions.cpp
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
    ${CMAKE_SOURCE_DIR}/plugins/rule_engines/re-irods/include
    ${CMAKE_BINARY_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/api/include
    ${CMAKE_SOURCE_DIR}/server/drivers/include
    ${CMAKE_SOURCE_DIR}/server/core/include
    ${CMAKE_SOURCE_DIR}/server/icat/include
    ${CMAKE_SOURCE_DIR}/server/re/include
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
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so
    dl 
    )

  target_compile_definitions(${PLUGIN} PRIVATE CACHE_ENABLE RODS_SERVER ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_RULE_ENGINE_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_HOME_DIRECTORY}/plugins/rule_engines
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
