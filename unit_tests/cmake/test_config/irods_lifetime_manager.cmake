set(IRODS_TEST_TARGET irods_lifetime_manager)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_irods_lifetime_manager.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common)
