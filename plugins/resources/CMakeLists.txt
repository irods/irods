set(
  IRODS_RESOURCE_PLUGIN_COMPOUND_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/compound/libcompound.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_DEFERRED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/deferred/libdeferred.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_LOAD_BALANCED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/load_balanced/libload_balanced.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_MOCKARCHIVE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/mockarchive/libmockarchive.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_NONBLOCKING_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking/libnonblocking.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_PASSTHRU_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/passthru/libpassthru.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_RANDOM_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/random/librandom.cpp
  )
set(IRODS_RESOURCE_PLUGIN_REPLICATION_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/replication/irods_create_write_replicator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/replication/irods_object_oper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/replication/irods_repl_rebalance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/replication/irods_repl_retry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/replication/irods_replicator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/replication/librepl.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_STRUCTFILE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/structfile/libstructfile.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_UNIVMSS_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/univmss/libunivmss.cpp
  )
set(
  IRODS_RESOURCE_PLUGIN_UNIXFILESYSTEM_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/unixfilesystem/libunixfilesystem.cpp
  )

set(
  IRODS_RESOURCE_PLUGINS
  compound
  deferred
  load_balanced
  mockarchive
  nonblocking
  passthru
  random
  replication
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
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    OpenSSL::Crypto
    )

  target_compile_definitions(${PLUGIN} PRIVATE ENABLE_RE ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED IRODS_ENABLE_SYSLOG)
endforeach()

install(
  TARGETS
  ${IRODS_RESOURCE_PLUGINS}
  LIBRARY
  DESTINATION ${IRODS_PLUGINS_DIRECTORY}/resources
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
