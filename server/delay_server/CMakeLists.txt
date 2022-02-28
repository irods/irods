cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_executable(
  irodsReServer
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irodsReServer.cpp"
)
target_link_libraries(
  irodsReServer
  PRIVATE
  irods_server
  irods_client
  irods_common
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_container.so"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/lib/libnanodbc.so"
  rt
  ${CMAKE_DL_LIBS}
  m
)
target_include_directories(
  irodsReServer
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
  "${IRODS_EXTERNALS_FULLPATH_NANODBC}/include"
  "${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include"
)
target_compile_definitions(
  irodsReServer
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG
)

install(
  TARGETS
  irodsReServer
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/irodsReServer.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
