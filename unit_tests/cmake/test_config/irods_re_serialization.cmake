set(IRODS_TEST_TARGET irods_re_serialization)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_re_serialization.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_server)
