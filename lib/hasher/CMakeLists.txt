set(
  IRODS_LIB_HASHER_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Hasher.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/MD5Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SHA256Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SHA512Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/ADLER32Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SHA1Strategy.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/checksum.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_hasher_factory.cpp"
)

set(
  IRODS_LIB_HASHER_HEADERS_PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/HashStrategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/Hasher.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/checksum.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/irods_hasher_factory.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/ADLER32Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/MD5Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/SHA1Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/SHA256Strategy.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/SHA512Strategy.hpp"
)

add_library(
  irods_hasher
  OBJECT
  "${IRODS_LIB_HASHER_SOURCES}"
  "${IRODS_LIB_HASHER_HEADERS_PUBLIC}"
)
objects_link_libraries(
  irods_hasher
  PUBLIC
  fmt::fmt
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
  FILES ${IRODS_LIB_HASHER_HEADERS_PUBLIC}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
