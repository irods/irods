set(IRODS_TEST_TARGET irods_zone_report)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_zone_report.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client)
