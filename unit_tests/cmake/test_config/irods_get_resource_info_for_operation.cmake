set(IRODS_TEST_TARGET irods_get_resource_info_for_operation)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_get_resource_info_for_operation.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              Boost::filesystem
                              Boost::system
                              fmt::fmt)
