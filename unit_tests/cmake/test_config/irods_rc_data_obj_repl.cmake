set(IRODS_TEST_TARGET irods_rc_data_obj_repl)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_rc_data_obj_repl.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              irods_plugin_dependencies
                              Boost::filesystem
                              Boost::system
                              fmt::fmt)
