set(IRODS_PLUGIN_TARGET msi_get_open_data_obj_l1desc_index)

add_library(${IRODS_PLUGIN_TARGET} MODULE libmsi_get_open_data_obj_l1desc_index.cpp)

target_compile_definitions(${IRODS_PLUGIN_TARGET} PRIVATE ENABLE_RE
                                                          ${IRODS_COMPILE_DEFINITIONS}
                                                          IRODS_ENABLE_SYSLOG)

target_include_directories(${IRODS_PLUGIN_TARGET} PRIVATE ${CMAKE_BINARY_DIR}/lib/core/include
                                                          ${CMAKE_SOURCE_DIR}/lib/core/include
                                                          ${CMAKE_SOURCE_DIR}/lib/api/include
                                                          ${CMAKE_SOURCE_DIR}/server/drivers/include
                                                          ${CMAKE_SOURCE_DIR}/server/api/include
                                                          ${CMAKE_SOURCE_DIR}/server/core/include
                                                          ${CMAKE_SOURCE_DIR}/server/icat/include
                                                          ${CMAKE_SOURCE_DIR}/server/re/include
                                                          ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)

target_link_libraries(${IRODS_PLUGIN_TARGET} PRIVATE irods_server
                                                     irods_common
                                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
                                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so)

install(TARGETS ${IRODS_PLUGIN_TARGET}
        LIBRARY DESTINATION ${IRODS_PLUGINS_DIRECTORY}/microservices
        COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME})
