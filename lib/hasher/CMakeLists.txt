cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_library(
  irods_hasher
  OBJECT
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Hasher.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/MD5Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SHA256Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SHA512Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ADLER32Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SHA1Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/checksum.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_hasher_factory.cpp"
)
target_link_libraries(
  irods_hasher
  PUBLIC
  OpenSSL::Crypto
  nlohmann_json::nlohmann_json
)
target_include_directories(
  irods_hasher
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
  irods_hasher
  PUBLIC
  ${IRODS_COMPILE_DEFINITIONS_PUBLIC}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG
)
set_target_properties(irods_hasher PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(irods_hasher PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/HashStrategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/Hasher.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/checksum.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/irods_hasher_factory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/ADLER32Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/MD5Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/SHA1Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/SHA256Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/SHA512Strategy.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
