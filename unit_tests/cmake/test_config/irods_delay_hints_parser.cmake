set(IRODS_TEST_TARGET irods_delay_hints_parser)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_delay_hints_parser.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              irods_plugin_dependencies
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
                              ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
