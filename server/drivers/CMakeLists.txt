set(
  IRODS_SERVER_DRIVERS_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/fileDriver.cpp"
)

set(
  IRODS_SERVER_DRIVERS_HEADERS_PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/fileDriver.hpp"
)

add_library(
  irods_server_drivers
  OBJECT
  "${IRODS_SERVER_DRIVERS_SOURCES}"
  "${IRODS_SERVER_DRIVERS_HEADERS_PUBLIC}"
)
objects_link_libraries(
  irods_server_drivers
  PUBLIC
  nlohmann_json::nlohmann_json
  fmt::fmt
  irods_common
  irods_plugin_dependencies
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_chrono.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
  OpenSSL::SSL
  OpenSSL::Crypto
  ${CMAKE_DL_LIBS}
  rt
)
target_include_directories(
  irods_server_drivers
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/plugins/api/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/api/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/core/include>"
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
)
target_compile_definitions(
  irods_server_drivers
  PUBLIC
  ${IRODS_COMPILE_DEFINITIONS_PUBLIC}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  ENABLE_RE
  RODS_CLERVER
  IRODS_ENABLE_SYSLOG
)
set_target_properties(irods_server_drivers PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_server_drivers PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

install(
  FILES ${IRODS_SERVER_DRIVERS_HEADERS_PUBLIC}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
