if (NOT ODBC_LIBRARY)
  find_library(ODBC_LIBRARY odbc)
  if (ODBC_LIBRARY)
    message(STATUS "Found odbc: ${ODBC_LIBRARY}")
  else()
    message(FATAL_ERROR "odbc library not found")
  endif()
endif()

set(
  IRODS_DATABASE_PLUGIN_POSTGRES_COMPILE_DEFINITIONS
  )
set(
  IRODS_DATABASE_PLUGIN_MYSQL_COMPILE_DEFINITIONS
  MY_ICAT
  )

set(
  IRODS_DATABASE_PLUGIN_POSTGRES_SCHEMA_UPDATE_FILES
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/2.postgres.mysql.oracle.sql
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/3.postgres.mysql.oracle.sql
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/4.postgres.oracle.sql
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/5.postgres.sql
  )
set(
  IRODS_DATABASE_PLUGIN_MYSQL_SCHEMA_UPDATE_FILES
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/2.postgres.mysql.oracle.sql
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/3.postgres.mysql.oracle.sql
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/4.mysql.sql
  ${CMAKE_SOURCE_DIR}/plugins/database/packaging/schema_updates/5.mysql.sql
  )

set(
  IRODS_DATABASE_PLUGIN_POSTGRES_DEFAULT_PORT
  5432
  )

set(
  IRODS_DATABASE_PLUGIN_MYSQL_DEFAULT_PORT
  3306
  )

set(
  IRODS_DATABASE_PLUGINS
  postgres
  mysql
  )

foreach(PLUGIN ${IRODS_DATABASE_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    COMMAND perl ${CMAKE_SOURCE_DIR}/plugins/database/src/convertSql.pl ${PLUGIN} ${CMAKE_SOURCE_DIR}/plugins/database/src ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/plugins/database/src/icatSysTables.sql.pp
    DEPENDS ${CMAKE_SOURCE_DIR}/plugins/database/src/convertSql.pl
    )
  add_custom_target(IRODS_PHONY_TARGET_icatSysTables_${PLUGIN}.sql ALL DEPENDS ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql) # Forces execution of custom_command

  set(IRODS_DATABASE_PLUGIN_TYPE ${PLUGIN})
  set(IRODS_DATABASE_DEFAULT_PORT ${IRODS_DATABASE_PLUGIN_${PLUGIN_UPPERCASE}_DEFAULT_PORT})
  configure_file(
    ${CMAKE_SOURCE_DIR}/plugins/database/packaging/setup_irods_database.sh.in
    ${CMAKE_BINARY_DIR}/plugins/database/packaging/setup_irods_database.sh.${PLUGIN}
    @ONLY
    )

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
    ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
    ${CMAKE_SOURCE_DIR}/plugins/database/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_clerver
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
    ${ODBC_LIBRARY}
    )

  target_compile_definitions(${PLUGIN} PRIVATE ENABLE_RE ${IRODS_DATABASE_PLUGIN_${PLUGIN_UPPERCASE}_COMPILE_DEFINITIONS} ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})

  install(
    TARGETS
    ${PLUGIN}
    LIBRARY
    DESTINATION ${IRODS_HOME_DIRECTORY}/plugins/database
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/${PLUGIN}/upgrade-3.3.xto4.0.0.sql
    ${CMAKE_SOURCE_DIR}/plugins/database/packaging/update_catalog_schema.py
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/packaging/find_odbc_${PLUGIN}.sh
    ${CMAKE_SOURCE_DIR}/plugins/database/packaging/find_bin_${PLUGIN}.sh
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
    )

  install(
    FILES
    ${CMAKE_BINARY_DIR}/plugins/database/packaging/setup_irods_database.sh.${PLUGIN}
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ
    RENAME setup_irods_database.sh
    )

  foreach(SCHEMA_UPDATE_SOURCE ${IRODS_DATABASE_PLUGIN_${PLUGIN_UPPERCASE}_SCHEMA_UPDATE_FILES})
    list(FIND IRODS_DATABASE_PLUGIN_${PLUGIN_UPPERCASE}_SCHEMA_UPDATE_FILES ${SCHEMA_UPDATE_SOURCE} SCHEMA_UPDATE_SOURCE_INDEX)
    math(EXPR SCHEMA_UPDATE_STEP "${SCHEMA_UPDATE_SOURCE_INDEX}+2")
    install(
      FILES
      ${SCHEMA_UPDATE_SOURCE}
      DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/schema_updates
      COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
      RENAME "${SCHEMA_UPDATE_STEP}.${PLUGIN}.sql"
      )
  endforeach()

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/packaging/localhost_setup_${PLUGIN}.input
    DESTINATION ${IRODS_HOME_DIRECTORY}/tests
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatDropSysTables.sql
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatSysInserts.sql
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatSetupValues.sql.template
    ${CMAKE_SOURCE_DIR}/plugins/database/src/icatPurgeRecycleBin.sql
    ${CMAKE_SOURCE_DIR}/plugins/database/src/convertSql.pl
    DESTINATION ${IRODS_HOME_DIRECTORY}/iRODS/server/icat/src
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/iRODS/server/icat/src
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    RENAME icatSysTables.sql
    )

  foreach(TEST_BINARY cll genq genu)
    add_executable(
      test_${TEST_BINARY}_${PLUGIN}
      ${CMAKE_SOURCE_DIR}/plugins/database/src/test_${TEST_BINARY}.cpp
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
      test_${TEST_BINARY}_${PLUGIN}
      PRIVATE
      ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
      ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
      ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
      ${CMAKE_SOURCE_DIR}/iRODS/lib/hasher/include
      ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
      ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
      ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
      ${CMAKE_SOURCE_DIR}/plugins/database/include
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
      ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
      )
    target_link_libraries(
      test_${TEST_BINARY}_${PLUGIN}
      PRIVATE
      irods_server
      irods_clerver
      ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
      ${ODBC_LIBRARY}
      )
    target_compile_definitions(test_${TEST_BINARY}_${PLUGIN} PRIVATE ${IRODS_DATABASE_PLUGIN_${PLUGIN_UPPERCASE}_COMPILE_DEFINITIONS} ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
    target_compile_options(test_${TEST_BINARY}_${PLUGIN} PRIVATE -Wno-write-strings)
    set_property(TARGET test_${TEST_BINARY}_${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
    set_property(TARGET test_${TEST_BINARY}_${PLUGIN} PROPERTY OUTPUT_NAME test_${TEST_BINARY})
    set_property(TARGET test_${TEST_BINARY}_${PLUGIN} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PLUGIN})

    install(
      TARGETS
      test_${TEST_BINARY}_${PLUGIN}
      RUNTIME
      DESTINATION ${IRODS_HOME_DIRECTORY}/iRODS/server/test/bin
      COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
      )
  endforeach()
endforeach()
