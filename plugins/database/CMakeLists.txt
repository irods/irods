if (NOT ODBC_LIBRARY)
  find_library(ODBC_LIBRARY odbc)
  if (ODBC_LIBRARY)
    message(STATUS "Found odbc: ${ODBC_LIBRARY}")
  else()
    message(FATAL_ERROR "odbc library not found")
  endif()
endif()

set(
  IRODS_DATABASE_PLUGIN_COMPILE_DEFINITIONS_postgres
  )
set(
  IRODS_DATABASE_PLUGIN_COMPILE_DEFINITIONS_mysql
  MY_ICAT
  )
set(
  IRODS_DATABASE_PLUGIN_COMPILE_DEFINITIONS_oracle
  ORA_ICAT
  )

set(
  IRODS_DATABASE_PLUGINS
  postgres
  mysql
  oracle
  )

configure_file(
  ${CMAKE_SOURCE_DIR}/plugins/database/src/icatSysTables.sql.pp.in
  ${CMAKE_BINARY_DIR}/plugins/database/src/icatSysTables.sql.pp
  @ONLY
  )
configure_file(
  ${CMAKE_SOURCE_DIR}/plugins/database/src/mysql_functions.sql.in
  ${CMAKE_BINARY_DIR}/plugins/database/src/mysql_functions.sql
  @ONLY
  )

foreach(PLUGIN ${IRODS_DATABASE_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    COMMAND cpp -E -P -D${PLUGIN} ${CMAKE_BINARY_DIR}/plugins/database/src/icatSysTables.sql.pp ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/plugins/database/src/icatSysTables.sql.pp
    )
  add_custom_target(IRODS_PHONY_TARGET_icatSysTables_${PLUGIN}.sql ALL DEPENDS ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql) # Forces execution of custom_command

  add_library(
    ${PLUGIN}
    MODULE
    ${CMAKE_SOURCE_DIR}/plugins/database/src/db_plugin.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/general_query.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/general_query_setup.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/general_update.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/irods_catalog_properties.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/irods_sql_logger.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/low_level_odbc.cpp
    ${CMAKE_SOURCE_DIR}/plugins/database/src/mid_level_routines.cpp
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    ${CMAKE_BINARY_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/api/include
    ${CMAKE_SOURCE_DIR}/lib/hasher/include
    ${CMAKE_SOURCE_DIR}/server/core/include
    ${CMAKE_SOURCE_DIR}/server/icat/include
    ${CMAKE_SOURCE_DIR}/server/re/include
    ${CMAKE_SOURCE_DIR}/plugins/database/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_plugin_dependencies
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    ${ODBC_LIBRARY}
    )

  target_compile_definitions(${PLUGIN} PRIVATE ENABLE_RE ${IRODS_DATABASE_PLUGIN_COMPILE_DEFINITIONS_${PLUGIN}} ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)

  install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION ${IRODS_PLUGINS_DIRECTORY}/database
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/${PLUGIN}/upgrade-3.3.xto4.0.0.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/packaging/localhost_setup_${PLUGIN}.input
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatDropSysTables.sql
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatSysInserts.sql
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatPurgeRecycleBin.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/sql
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

  install(
    FILES
    ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/sql
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    RENAME icatSysTables.sql
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

endforeach()

install(
  FILES
  ${CMAKE_BINARY_DIR}/plugins/database/src/mysql_functions.sql
  DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/sql
  COMPONENT ${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}
  )
