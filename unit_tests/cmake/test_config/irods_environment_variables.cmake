set(IRODS_TEST_TARGET irods_environment_variables)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_environment_variables.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_server)
