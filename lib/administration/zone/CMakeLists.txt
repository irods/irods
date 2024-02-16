cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

foreach(variant IN ITEMS client server)
  set(target_name "irods_zone_administration_${variant}")

  add_library(
    ${target_name}
    OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zone_administration.cpp"
  )
  target_link_libraries(
    ${target_name}
    PUBLIC
    fmt::fmt
    PRIVATE
    nlohmann_json::nlohmann_json
  )
  target_include_directories(
    ${target_name}
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
  irods_zone_administration_server
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/core/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/api/include>"
)
target_compile_definitions(
  irods_zone_administration_server
  PRIVATE
  IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/zone_administration.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/zone_type.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)

