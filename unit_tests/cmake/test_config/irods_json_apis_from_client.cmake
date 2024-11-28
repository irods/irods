set(IRODS_TEST_TARGET irods_json_apis_from_client)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_json_apis_from_client.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              Boost::filesystem
                              Boost::system
                              fmt::fmt)
