cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_library(
  irods_user_administration_common
  OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/src/group.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/user.cpp"
)
target_link_libraries(
  irods_user_administration_common
  PRIVATE
  nlohmann_json::nlohmann_json
)
target_include_directories(
  irods_user_administration_common
  PUBLIC
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_compile_definitions(
  irods_user_administration_common
  PUBLIC
  ${IRODS_COMPILE_DEFINITIONS_PUBLIC}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
)
set_target_properties(irods_user_administration_common PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_user_administration_common PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

foreach(variant IN ITEMS client server)
  set(target_name "irods_user_administration_${variant}")

  add_library(
    ${target_name}
    OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/src/user_administration.cpp"
  )
  target_link_libraries(
    ${target_name}
    PUBLIC
    irods_user_administration_common
    PRIVATE
    nlohmann_json::nlohmann_json
  )
  target_include_directories(
    ${target_name}
    PUBLIC
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_IRODS_BINARY_DIR}/lib/core/include>"
    "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/lib/core/include>"
    "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/lib/api/include>"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
    "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
  )
  target_compile_definitions(
    ${target_name}
    PUBLIC
    ${IRODS_COMPILE_DEFINITIONS_PUBLIC}
    PRIVATE
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  )
  set_target_properties(${target_name} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
  set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endforeach()
target_include_directories(
  irods_user_administration_server
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/core/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/api/include>"
)
target_compile_definitions(
  irods_user_administration_server
  PRIVATE
  IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/group.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/user.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/user_administration.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
