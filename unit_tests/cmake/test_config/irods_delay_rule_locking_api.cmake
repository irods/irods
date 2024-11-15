set(IRODS_TEST_TARGET irods_delay_rule_locking_api)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_delay_rule_locking_api.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_client
                              irods_common
                              fmt::fmt
                              nlohmann_json::nlohmann_json)
