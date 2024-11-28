set(IRODS_TEST_TARGET irods_data_object_finalize)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_data_object_finalize.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              irods_plugin_dependencies
                              Boost::filesystem
                              Boost::system
                              fmt::fmt)
