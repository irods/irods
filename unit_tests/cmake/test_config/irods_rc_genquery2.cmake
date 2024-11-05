set(IRODS_TEST_TARGET irods_rc_genquery2)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_rc_genquery2.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              Boost::filesystem
                              Boost::system
                              fmt::fmt)
