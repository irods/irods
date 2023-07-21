set(IRODS_TEST_TARGET irods_rcConnect)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_rcConnect.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_client)
