set(
  IRODS_MICROSERVICE_ADMINISTRATION_PLUGINS
  msisync_to_archive
  msi_update_unixfilesystem_resource_free_space
  )

foreach(PLUGIN ${IRODS_MICROSERVICE_ADMINISTRATION_PLUGINS})
  add_library(
    ${PLUGIN}
    MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN}/lib${PLUGIN}.cpp
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    )

  target_compile_definitions(${PLUGIN} PRIVATE ENABLE_RE ${IRODS_COMPILE_DEFINITIONS} IRODS_ENABLE_SYSLOG)
endforeach()

install(
  TARGETS
    ${IRODS_MICROSERVICE_ADMINISTRATION_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_PLUGINS_DIRECTORY}/microservices
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
