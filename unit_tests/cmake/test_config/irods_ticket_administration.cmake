set(IRODS_TEST_TARGET irods_ticket_administration)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_ticket_administration.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_FMT}/include)
                             
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client
                              ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
