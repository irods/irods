find_package(CURL REQUIRED)

set(
  IRODS_MICROSERVICE_MSO_PLUGINS
  msiobjget_http
  msiobjget_irods
  msiobjget_slink
  msiobjput_http
  msiobjput_irods
  msiobjput_slink
  )

foreach(PLUGIN ${IRODS_MICROSERVICE_MSO_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_library(
    ${PLUGIN}
    MODULE
    ${CMAKE_SOURCE_DIR}/plugins/microservices/mso_drivers/${PLUGIN}/lib${PLUGIN}.cpp
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/plugins/microservices/mso_drivers
    ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
    ${CURL_INCLUDE_DIRS}
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    irods_server
    irods_clerver
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${CURL_LIBRARIES}
    )

  target_compile_definitions(${PLUGIN} PRIVATE RODS_SERVER ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_MICROSERVICE_MSO_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_HOME_DIRECTORY}/plugins/microservices
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
