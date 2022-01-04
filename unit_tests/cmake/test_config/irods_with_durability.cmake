set(IRODS_TEST_TARGET irods_with_durability)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_with_durability.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/lib/core/include)
 
set(IRODS_TEST_LINK_LIBRARIES "")
