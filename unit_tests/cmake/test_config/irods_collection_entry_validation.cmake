set(IRODS_TEST_TARGET irods_collection_entry_validation)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_collection_entry_validation.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common) # Provides the iRODS headers.
