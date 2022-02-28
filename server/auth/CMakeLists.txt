cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

if (NOT PAM_LIBRARY)
  find_library(PAM_LIBRARY pam)
  if (PAM_LIBRARY)
    message(STATUS "Found pam: ${PAM_LIBRARY}")
  else()
    message(FATAL_ERROR "pam library not found")
  endif()
endif()

add_executable(
  irodsPamAuthCheck
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irodsPamAuthCheck.cpp"
)
target_link_libraries(
  irodsPamAuthCheck
  PRIVATE
  "${PAM_LIBRARY}"
)

install(
  TARGETS
  irodsPamAuthCheck
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  PERMISSIONS SETUID OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

add_executable(
  genOSAuth
  "${CMAKE_CURRENT_SOURCE_DIR}/src/genOSAuth.cpp"
)
target_link_libraries(
  genOSAuth
  PRIVATE
  irods_client
  irods_plugin_dependencies
  irods_common
  "${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
  "${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so"
)
target_compile_definitions(
  genOSAuth
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
)

install(
  TARGETS
  genOSAuth
  RUNTIME
  DESTINATION "${IRODS_HOME_DIRECTORY}/clients/bin"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
