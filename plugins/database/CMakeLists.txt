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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/icatSysTables.sql.pp.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/icatSysTables.sql.pp
  @ONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mysql_functions.sql.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/mysql_functions.sql
  @ONLY
  )

foreach(PLUGIN ${IRODS_DATABASE_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    COMMAND cpp -E -P -D${PLUGIN} ${CMAKE_CURRENT_BINARY_DIR}/src/icatSysTables.sql.pp ${CMAKE_CURRENT_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/src/icatSysTables.sql.pp
    )
  add_custom_target(IRODS_PHONY_TARGET_icatSysTables_${PLUGIN}.sql ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/icatSysTables_${PLUGIN}.sql) # Forces execution of custom_command

  add_library(
    ${PLUGIN}
    MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/db_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/general_query.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/general_query_setup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/general_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/irods_catalog_properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/irods_sql_logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/low_level_odbc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mid_level_routines.cpp
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
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

  target_compile_definitions(${PLUGIN} PRIVATE ENABLE_RE ${IRODS_DATABASE_PLUGIN_COMPILE_DEFINITIONS_${PLUGIN}} ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED IRODS_ENABLE_SYSLOG)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN}/upgrade-3.3.xto4.0.0.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    )

  install(
    FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/localhost_setup_${PLUGIN}.input
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

  install(
    FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/icatDropSysTables.sql
    ${CMAKE_CURRENT_SOURCE_DIR}/src/icatSysInserts.sql
    ${CMAKE_CURRENT_SOURCE_DIR}/src/icatPurgeRecycleBin.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/sql
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/icatSysTables_${PLUGIN}.sql
    DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/sql
    COMPONENT ${IRODS_PACKAGE_COMPONENT_${PLUGIN_UPPERCASE}_NAME}
    RENAME icatSysTables.sql
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

endforeach()

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/src/mysql_functions.sql
  DESTINATION ${IRODS_HOME_DIRECTORY}/packaging/sql
  COMPONENT ${IRODS_PACKAGE_COMPONENT_MYSQL_NAME}
  )
