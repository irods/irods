set(IRODS_TEST_TARGET irods_scoped_privileged_client)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_scoped_privileged_client.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_server)
