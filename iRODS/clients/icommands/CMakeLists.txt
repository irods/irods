set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/iRODS/clients/icommands/src/irods-grid.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/iRODS/lib/core/include/server_control_plane_command.hpp
  )

set(
  IRODS_CLIENTS_ICOMMANDS_EXECUTABLES
  genOSAuth
  iadmin
  iapitest
  ibun
  icd
  ichksum
  ichmod
  icp
  idbug
  ienv
  ierror
  iexecmd
  iexit
  ifsck
  iget
  igroupadmin
  ihelp
  iinit
  ils
  ilsresc
  imcoll
  imeta
  imiscsvrinfo
  imkdir
  imv
  ipasswd
  iphybun
  iphymv
  ips
  iput
  ipwd
  iqdel
  iqmod
  iqstat
  iquest
  iquota
  ireg
  irepl
  irm
  irmtrash
  irods-grid
  irsync
  irule
  iscan
  isysmeta
  iticket
  itrim
  iuserinfo
  ixmsg
  izonereport
  )

foreach(EXECUTABLE ${IRODS_CLIENTS_ICOMMANDS_EXECUTABLES})
  add_executable(
    ${EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/iRODS/clients/icommands/src/${EXECUTABLE}.cpp
    )
  target_link_libraries(
    ${EXECUTABLE}
    PRIVATE
    irods_client_core
    irods_client_plugins
    irods_client_api_table
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/lib/libavrocpp.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/lib/libjansson.so
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/lib/libzmq.so
    )
    target_include_directories(
    ${EXECUTABLE}
    PRIVATE
    ${CMAKE_BINARY_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/lib/api/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/drivers/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/core/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/icat/include
    ${CMAKE_SOURCE_DIR}/iRODS/server/re/include
    ${IRODS_EXTERNALS_FULLPATH_AVRO}/include
    ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
    ${IRODS_EXTERNALS_FULLPATH_JANSSON}/include
    ${IRODS_EXTERNALS_FULLPATH_ARCHIVE}/include
    ${IRODS_EXTERNALS_FULLPATH_CPPZMQ}/include
    ${IRODS_EXTERNALS_FULLPATH_ZMQ}/include
    )
  target_compile_definitions(${EXECUTABLE} PRIVATE RODS_SERVER ${IRODS_COMPILE_DEFINITIONS} BOOST_SYSTEM_NO_DEPRECATED)
  target_compile_options(${EXECUTABLE} PRIVATE -Wno-write-strings)
  set_property(TARGET ${EXECUTABLE} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
endforeach()

install(
  TARGETS
    ${IRODS_CLIENTS_ICOMMANDS_EXECUTABLES}
  RUNTIME
    DESTINATION bin
    COMPONENT ${IRODS_PACKAGE_COMPONENT_ICOMMANDS_NAME}
  )
