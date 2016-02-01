find_package(OpenSSL REQUIRED)

set(
  IRODS_RESOURCE_PLUGIN_COMPOUND_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/compound/libcompound.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_DEFERRED_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/deferred/libdeferred.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_LOAD_BALANCED_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/load_balanced/libload_balanced.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_MOCKARCHIVE_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/mockarchive/libmockarchive.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_MSO_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/mso/libmso.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_NONBLOCKING_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/nonblocking/libnonblocking.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_PASSTHRU_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/passthru/libpassthru.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_RANDOM_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/random/librandom.cpp
  )
set(IRODS_RESOURCE_PLUGIN_REPLICATION_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/replication/irods_create_write_replicator.cpp
  ${CMAKE_SOURCE_DIR}/plugins/resources/replication/irods_object_oper.cpp
  ${CMAKE_SOURCE_DIR}/plugins/resources/replication/irods_repl_rebalance.cpp
  ${CMAKE_SOURCE_DIR}/plugins/resources/replication/irods_replicator.cpp
  ${CMAKE_SOURCE_DIR}/plugins/resources/replication/irods_unlink_replicator.cpp
  ${CMAKE_SOURCE_DIR}/plugins/resources/replication/librepl.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_ROUNDROBIN_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/roundrobin/libroundrobin.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_STRUCTFILE_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/structfile/libstructfile.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_UNIVMSS_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/univmss/libunivmss.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_UNIXFILESYSTEM_SOURCES
  ${CMAKE_SOURCE_DIR}/plugins/resources/unixfilesystem/libunixfilesystem.cpp
  )

set(
  IRODS_RESOURCE_PLUGINS
  compound
  deferred
  load_balanced
  mockarchive
  mso
  nonblocking
  passthru
  random
  replication
  roundrobin
  structfile
  univmss
  unixfilesystem
  )

foreach(PLUGIN ${IRODS_RESOURCE_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_library(
    ${PLUGIN}
    MODULE
    ${IRODS_RESOURCE_PLUGIN_${PLUGIN_UPPERCASE}_SOURCES}
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
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_clerver
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so
    ${OPENSSL_CRYPTO_LIBRARY}
    )

  target_compile_definitions(${PLUGIN} PRIVATE RODS_SERVER ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_RESOURCE_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_HOME_DIRECTORY}/plugins/resources
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
