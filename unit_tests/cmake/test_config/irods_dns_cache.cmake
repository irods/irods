set(IRODS_TEST_TARGET irods_dns_cache)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_dns_cache.cpp)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common)
