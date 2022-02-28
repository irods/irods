cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_library(
  irods_rbudp
  OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/src/QUANTAnet_rbudpBase_c.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/QUANTAnet_rbudpReceiver_c.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/QUANTAnet_rbudpSender_c.cpp"
)
target_link_libraries(
  irods_rbudp
  PUBLIC
  nlohmann_json::nlohmann_json
)
target_include_directories(
  irods_rbudp
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_BINARY_DIR}/lib/core/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/lib/core/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/lib/api/include>"
  PRIVATE
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
)
target_compile_definitions(
  irods_rbudp
  PUBLIC
  ${IRODS_COMPILE_DEFINITIONS_PUBLIC}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG
)
set_target_properties(irods_rbudp PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_rbudp PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/QUANTAnet_rbudpBase_c.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/QUANTAnet_rbudpReceiver_c.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/QUANTAnet_rbudpSender_c.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
