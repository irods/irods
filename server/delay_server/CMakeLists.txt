add_executable(
  irodsDelayServer
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irodsDelayServer.cpp"
)
target_link_libraries(
  irodsDelayServer
  PRIVATE
  fmt::fmt
  Boost::headers
  Boost::system
  Boost::filesystem
  Boost::thread
  Boost::container
  irods_server
  irods_client
  irods_common
  rt
  ${CMAKE_DL_LIBS}
  m
)
target_include_directories(
  irodsDelayServer
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_compile_definitions(
  irodsDelayServer
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG
)

add_dependencies(all-server irodsDelayServer)
install(
  TARGETS
  irodsDelayServer
  RUNTIME
  DESTINATION "${CMAKE_INSTALL_SBINDIR}"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/irods/irodsDelayServer.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/irods"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_DEVELOPMENT_NAME}
)
