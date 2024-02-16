set(IRODS_TEST_TARGET irods_get_delay_rule_info)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_get_delay_rule_info.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_client
                              irods_common
                              ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
                              nlohmann_json::nlohmann_json)
