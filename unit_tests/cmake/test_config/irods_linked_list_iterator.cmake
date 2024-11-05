set(IRODS_TEST_TARGET irods_linked_list_iterator)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_irods_linked_list_iterator.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_server
                              Boost::system)
