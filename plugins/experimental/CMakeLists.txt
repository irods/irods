find_package(OpenSSL REQUIRED)

set(
  IRODS_EXPERIMENTAL_API_PLUGIN_REMOVE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/remove.cpp
  )

set(
  IRODS_EXPERIMENTAL_API_PLUGIN_REPLICATE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/replicate.cpp
  )

set(
  IRODS_EXPERIMENTAL_API_PLUGIN_COPY_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/copy.cpp
  )

set(
  IRODS_EXPERIMENTAL_API_PLUGIN_REPLICATE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/replicate.cpp
  )

set(
  IRODS_EXPERIMENTAL_API_PLUGINS
  copy
  remove
  replicate
  )

foreach(PLUGIN ${IRODS_EXPERIMENTAL_API_PLUGINS})
  string(TOUPPER ${PLUGIN} PLUGIN_UPPERCASE)

  add_library(
    ${PLUGIN}
    MODULE
    ${IRODS_EXPERIMENTAL_API_PLUGIN_${PLUGIN_UPPERCASE}_SOURCES}
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    ${CMAKE_BINARY_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/filesystem/include
    ${CMAKE_SOURCE_DIR}/lib/api/include
    ${CMAKE_SOURCE_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/server/api/include
    ${CMAKE_SOURCE_DIR}/server/core/include
    ${CMAKE_SOURCE_DIR}/server/drivers/include
    ${CMAKE_SOURCE_DIR}/server/icat/include
    ${CMAKE_SOURCE_DIR}/server/re/include
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
    ${CMAKE_DL_LIBS}
    Threads::Threads
    )

  target_compile_definitions(${PLUGIN} PRIVATE ENABLE_RE ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED IRODS_ENABLE_SYSLOG)
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings)
endforeach()

install(
  TARGETS
  ${IRODS_EXPERIMENTAL_API_PLUGINS}
  LIBRARY
  DESTINATION ${IRODS_PLUGINS_DIRECTORY}/experimental
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
