set(
  IRODS_PLUGIN_TARGETS
  msi_test_issue_6782
  msi_test_issue_6829
  msi_test_scoped_client_identity
)

add_compile_definitions(
  ENABLE_RE
  ${IRODS_COMPILE_DEFINITIONS}
)

include_directories(
  ${CMAKE_BINARY_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/core/include
  ${CMAKE_SOURCE_DIR}/lib/api/include
  ${CMAKE_SOURCE_DIR}/lib/filesystem/include
  ${CMAKE_SOURCE_DIR}/server/drivers/include
  ${CMAKE_SOURCE_DIR}/server/api/include
  ${CMAKE_SOURCE_DIR}/server/core/include
  ${CMAKE_SOURCE_DIR}/server/icat/include
  ${CMAKE_SOURCE_DIR}/server/re/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
  ${IRODS_EXTERNALS_FULLPATH_FMT}/include
)

link_libraries(
  irods_server
  irods_common
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
  ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
)

foreach (IRODS_PLUGIN IN LISTS IRODS_PLUGIN_TARGETS)
  add_library(${IRODS_PLUGIN} MODULE lib${IRODS_PLUGIN}.cpp)
endforeach()

install(
  TARGETS ${IRODS_PLUGIN_TARGETS}
  LIBRARY DESTINATION ${IRODS_PLUGINS_DIRECTORY}/microservices
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
