set(IRODS_TEST_TARGET irods_version)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_version.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common)
