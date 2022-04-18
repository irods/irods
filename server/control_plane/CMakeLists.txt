cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/irods/"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/include/irods/"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/irods/server_control_plane_command.hpp"
  COMMAND "${CMAKE_COMMAND}" -E env LD_LIBRARY_PATH="${IRODS_EXTERNALS_FULLPATH_BOOST}/lib" "${IRODS_EXTERNALS_FULLPATH_AVRO}/bin/avrogencpp" -n irods -o "${CMAKE_CURRENT_BINARY_DIR}/include/irods/server_control_plane_command.hpp" -i "${CMAKE_IRODS_SOURCE_DIR}/schemas/messaging/v1/server_control_plane_command.json"
  MAIN_DEPENDENCY "${CMAKE_IRODS_SOURCE_DIR}/schemas/messaging/v1/server_control_plane_command.json"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/include/irods/"
)

add_custom_target(
  irods_server_control_plane_command_hpp
  ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/include/irods/server_control_plane_command.hpp"
)

add_dependencies(all-server irods_server_control_plane_command_hpp)
install(
  FILES
  "${CMAKE_CURRENT_BINARY_DIR}/include/irods/server_control_plane_command.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)

add_library(
  irods_server_control_plane
  INTERFACE
)
target_link_libraries(
  irods_server_control_plane
  INTERFACE
  irods_common
)
target_include_directories(
  irods_server_control_plane
  INTERFACE
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
)
add_dependencies(
  irods_server_control_plane
  irods_server_control_plane_command_hpp
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/irods_server_control_plane.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
