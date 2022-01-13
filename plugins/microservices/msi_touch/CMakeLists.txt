set(IRODS_PLUGIN_TARGET msi_touch)

add_library(${IRODS_PLUGIN_TARGET} MODULE libmsi_touch.cpp)

target_compile_definitions(${IRODS_PLUGIN_TARGET} PRIVATE ENABLE_RE
                                                          ${IRODS_COMPILE_DEFINITIONS}
                                                          IRODS_ENABLE_SYSLOG)

target_include_directories(${IRODS_PLUGIN_TARGET} PRIVATE ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                                                          ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

target_link_libraries(${IRODS_PLUGIN_TARGET} PRIVATE irods_server
                                                     irods_common
                                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
                                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
                                                     ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)

install(TARGETS ${IRODS_PLUGIN_TARGET}
        LIBRARY DESTINATION ${IRODS_PLUGINS_DIRECTORY}/microservices
        COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME})
