set(IRODS_TEST_TARGET irods_file_object)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_file_object.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_server)
