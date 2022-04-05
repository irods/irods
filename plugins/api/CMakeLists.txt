cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

# create phony target to build everything in plugins/api folder
add_custom_target(all-plugins-api)
add_dependencies(all-plugins_no_database all-plugins-api)

set(
  IRODS_API_PLUGINS
  experimental_api_plugin_adaptor
  helloworld
  atomic_apply_acl_operations
  atomic_apply_metadata_operations
  authenticate
  data_object_finalize
  data_object_modify_info
  get_file_descriptor_info
  register_physical_path
  replica_close
  replica_open
  touch
)

set(
  IRODS_API_PLUGINS_NONPREFIXED
  experimental_api_plugin_adaptor
  helloworld
)

foreach(plugin IN LISTS IRODS_API_PLUGINS)
  set(plugin_target_part "irods_api_plugin-${plugin}")
  if (plugin IN_LIST IRODS_API_PLUGINS_NONPREFIXED)
    set(plugin_name_part "${plugin}")
  else()
    set(plugin_name_part "irods_${plugin}")
  endif()

  foreach(variant IN ITEMS client server)
    set(plugin_target "${plugin_target_part}_${variant}")
    set(plugin_name "${plugin_name_part}_${variant}")

    add_library(
      ${plugin_target}
      MODULE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/${plugin}.cpp"
    )
    set_property(TARGET ${plugin_target} PROPERTY LIBRARY_OUTPUT_NAME ${plugin_name})
    target_link_libraries(
      ${plugin_target}
      PRIVATE
      irods_plugin_dependencies
      irods_common
      irods_${variant}
      "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/lib/libarchive.so"
      "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
      "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
      "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
      "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
      nlohmann_json::nlohmann_json
    )
    target_include_directories(
      ${plugin_target}
      PRIVATE
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
      "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/server/api/include>"
      "${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include"
      "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
      "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
      "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
      "${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include"
    )
    target_compile_definitions(
      ${plugin_target}
      PRIVATE
      ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    )

    add_dependencies(all-plugins-api ${plugin_target})
    install(
      TARGETS
      ${plugin_target}
      LIBRARY
      DESTINATION "${IRODS_PLUGINS_DIRECTORY}/api"
      COMPONENT ${IRODS_PACKAGE_COMPONENT_RUNTIME_NAME}
    )
  endforeach()

  target_compile_definitions(
    ${plugin_target_part}_server
    PRIVATE
    RODS_SERVER
    ENABLE_RE
    IRODS_ENABLE_SYSLOG
  )
endforeach()

target_link_libraries(
  irods_api_plugin-atomic_apply_acl_operations_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
)
target_include_directories(
  irods_api_plugin-atomic_apply_acl_operations_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
)

target_link_libraries(
  irods_api_plugin-authenticate_client
  PRIVATE
  ${CMAKE_DL_LIBS}
)
target_link_libraries(
  irods_api_plugin-authenticate_server
  PRIVATE
  ${CMAKE_DL_LIBS}
)

target_link_libraries(
  irods_api_plugin-data_object_finalize_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
)
target_include_directories(
  irods_api_plugin-data_object_finalize_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
)

target_link_libraries(
  irods_api_plugin-atomic_apply_metadata_operations_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
)
target_include_directories(
  irods_api_plugin-atomic_apply_metadata_operations_server
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
)

target_link_libraries(
  irods_api_plugin-experimental_api_plugin_adaptor_client
  PRIVATE
  ${CMAKE_DL_LIBS}
)
target_link_libraries(
  irods_api_plugin-experimental_api_plugin_adaptor_server
  PRIVATE
  ${CMAKE_DL_LIBS}
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/plugins/api/api_plugin_number.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/plugins/api/api_plugin_number_map.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/plugins/api/api_plugin_number_data.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods/plugins/api"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
