set(IRODS_TEST_TARGET irods_system_error)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_system_error.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              fmt::fmt)
