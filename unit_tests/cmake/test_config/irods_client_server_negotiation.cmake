set(IRODS_TEST_TARGET irods_client_server_negotiation)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_client_server_negotiation.cpp)

set(IRODS_TEST_INCLUDE_PATH nlohmann_json::nlohmann_json)

set(IRODS_TEST_LINK_LIBRARIES irods_client
                              nlohmann_json::nlohmann_json
                              Boost::system)
