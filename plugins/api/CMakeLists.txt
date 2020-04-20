find_package(OpenSSL REQUIRED)

# helloworld API
set(
  IRODS_API_PLUGIN_SOURCES_helloworld_server
  ${CMAKE_SOURCE_DIR}/plugins/api/src/helloworld.cpp
  )

set(
  IRODS_API_PLUGIN_SOURCES_helloworld_client
  ${CMAKE_SOURCE_DIR}/plugins/api/src/helloworld.cpp
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_helloworld_server
  RODS_SERVER
  ENABLE_RE
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_helloworld_client
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_helloworld_server
  irods_server
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_helloworld_client
  irods_client
  )

# get_file_descriptor_info API
set(
  IRODS_API_PLUGIN_SOURCES_irods_get_file_descriptor_info_server
  ${CMAKE_SOURCE_DIR}/plugins/api/src/get_file_descriptor_info.cpp
  )

set(
  IRODS_API_PLUGIN_SOURCES_irods_get_file_descriptor_info_client
  ${CMAKE_SOURCE_DIR}/plugins/api/src/get_file_descriptor_info.cpp
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_irods_get_file_descriptor_info_server
  RODS_SERVER
  ENABLE_RE
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_irods_get_file_descriptor_info_client
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_irods_get_file_descriptor_info_server
  irods_server
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_irods_get_file_descriptor_info_client
  irods_client
  )

# data_object_modify_info API
set(
  IRODS_API_PLUGIN_SOURCES_irods_data_object_modify_info_server
  ${CMAKE_SOURCE_DIR}/plugins/api/src/data_object_modify_info.cpp
  )

set(
  IRODS_API_PLUGIN_SOURCES_irods_data_object_modify_info_client
  ${CMAKE_SOURCE_DIR}/plugins/api/src/data_object_modify_info.cpp
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_irods_data_object_modify_info_server
  RODS_SERVER
  ENABLE_RE
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_irods_data_object_modify_info_client
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_irods_data_object_modify_info_server
  irods_server
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_irods_data_object_modify_info_client
  irods_client
  )

# atomic_apply_metadata_operations API
set(
  IRODS_API_PLUGIN_SOURCES_irods_atomic_apply_metadata_operations_server
  ${CMAKE_SOURCE_DIR}/plugins/api/src/atomic_apply_metadata_operations.cpp
  )

set(
  IRODS_API_PLUGIN_SOURCES_irods_atomic_apply_metadata_operations_client
  ${CMAKE_SOURCE_DIR}/plugins/api/src/atomic_apply_metadata_operations.cpp
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_irods_atomic_apply_metadata_operations_server
  RODS_SERVER
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
  )

set(
  IRODS_API_PLUGIN_COMPILE_DEFINITIONS_irods_atomic_apply_metadata_operations_client
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_irods_atomic_apply_metadata_operations_server
  irods_server
  ${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so
  )

set(
  IRODS_API_PLUGIN_LINK_LIBRARIES_irods_atomic_apply_metadata_operations_client
  irods_client
  )

set(
  IRODS_API_PLUGINS
  helloworld_server
  helloworld_client
  irods_get_file_descriptor_info_server
  irods_get_file_descriptor_info_client
  irods_data_object_modify_info_server
  irods_data_object_modify_info_client
  irods_atomic_apply_metadata_operations_server
  irods_atomic_apply_metadata_operations_client
  )

foreach(PLUGIN ${IRODS_API_PLUGINS})
  add_library(
    ${PLUGIN}
    MODULE
    ${IRODS_API_PLUGIN_SOURCES_${PLUGIN}}
    )

  target_include_directories(
    ${PLUGIN}
    PRIVATE
    ${CMAKE_BINARY_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/core/include
    ${CMAKE_SOURCE_DIR}/lib/api/include
    ${CMAKE_SOURCE_DIR}/lib/filesystem/include
    ${CMAKE_SOURCE_DIR}/plugins/api/include
    ${CMAKE_SOURCE_DIR}/server/api/include
    ${CMAKE_SOURCE_DIR}/server/core/include
    ${CMAKE_SOURCE_DIR}/server/drivers/include
    ${CMAKE_SOURCE_DIR}/server/icat/include
    ${CMAKE_SOURCE_DIR}/server/re/include
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    ${IRODS_EXTERNALS_FULLPATH_JSON}/include
    ${IRODS_EXTERNALS_FULLPATH_NANODBC}/include
    )

  target_link_libraries(
    ${PLUGIN}
    PRIVATE
    ${IRODS_API_PLUGIN_LINK_LIBRARIES_${PLUGIN}}
    irods_plugin_dependencies
    irods_common
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
    ${OPENSSL_CRYPTO_LIBRARY}
    )

  target_compile_definitions(${PLUGIN} PRIVATE ${IRODS_API_PLUGIN_COMPILE_DEFINITIONS_${PLUGIN}} ${IRODS_COMPILE_DEFINITIONS})
  target_compile_options(${PLUGIN} PRIVATE -Wno-write-strings -Wno-ignored-qualifiers)
  set_property(TARGET ${PLUGIN} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_API_PLUGINS}
  LIBRARY
    DESTINATION ${IRODS_PLUGINS_DIRECTORY}/api
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
