set(IRODS_TEST_TARGET irods_rcTicketAdmin)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_rcTicketAdmin.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client)
