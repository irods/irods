set(IRODS_TEST_TARGET irods_hashers)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_hashers.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              fmt::fmt)
