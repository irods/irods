foreach(harness IN ITEMS api configuration)
  add_executable(
    irods_${harness}_test_harness
    "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_${harness}_test_harness.cpp"
  )
  target_link_libraries(
    irods_${harness}_test_harness
    PRIVATE
    irods_server
    irods_client
    irods_plugin_dependencies
    irods_common
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_thread.so"
    rt
    ${CMAKE_DL_LIBS}
    m
  )
  target_include_directories(
    irods_${harness}_test_harness
    PRIVATE
    "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  )
  target_compile_definitions(
    irods_${harness}_test_harness
    PRIVATE
    ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
    IRODS_ENABLE_SYSLOG
  )

  add_dependencies(all-server irods_${harness}_test_harness)
  install(
    TARGETS
    irods_${harness}_test_harness
    RUNTIME
    DESTINATION "${CMAKE_INSTALL_SBINDIR}"
    COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
  )
endforeach()
