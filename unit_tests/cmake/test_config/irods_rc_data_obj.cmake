set(IRODS_TEST_TARGET irods_rc_data_obj)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_rc_data_obj.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_BINARY_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/api/include
                            ${CMAKE_SOURCE_DIR}/lib/filesystem/include
                            ${CMAKE_SOURCE_DIR}/server/core/include
                            ${CMAKE_SOURCE_DIR}/server/icat/include
                            ${CMAKE_SOURCE_DIR}/server/re/include
                            ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              irods_plugin_dependencies
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
                              ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
