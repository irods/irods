find_package(FLEX 2.6.1 REQUIRED)
find_package(BISON 3.0.4 REQUIRED)

set(IRODS_FLEX_LEXER_NAME irods_lexer)
set(IRODS_BISON_PARSER_NAME irods_parser)

FLEX_TARGET(${IRODS_FLEX_LEXER_NAME} dsl/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
BISON_TARGET(${IRODS_BISON_PARSER_NAME} dsl/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
ADD_FLEX_BISON_DEPENDENCY(${IRODS_FLEX_LEXER_NAME} ${IRODS_BISON_PARSER_NAME})

set(IRODS_PARSER_NAME irods_genquery2_parser)
set(IRODS_FLEX_OUTPUTS "FLEX_${IRODS_FLEX_LEXER_NAME}_OUTPUTS")
set(IRODS_BISON_OUTPUTS "BISON_${IRODS_BISON_PARSER_NAME}_OUTPUTS")

add_library(
  ${IRODS_PARSER_NAME}
  OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/src/genquery2_driver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/genquery2_sql.cpp
  ${${IRODS_FLEX_OUTPUTS}}
  ${${IRODS_BISON_OUTPUTS}}
)

set_target_properties(${IRODS_PARSER_NAME} PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(${IRODS_PARSER_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_compile_definitions(
  ${IRODS_PARSER_NAME}
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS}
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  IRODS_ENABLE_SYSLOG
)

# Defined here since it's isolated to this specific target.
target_compile_options(
  ${IRODS_PARSER_NAME}
  PRIVATE
  "-Wno-overloaded-virtual"
)

target_include_directories(
  ${IRODS_PARSER_NAME}
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
)

target_link_libraries(
  ${IRODS_PARSER_NAME}
  PUBLIC
  nlohmann_json::nlohmann_json
)

if (${IRODS_VERSION} VERSION_GREATER "4.3.1")
  target_link_libraries(
    ${IRODS_PARSER_NAME}
    PRIVATE
    irods_common
    fmt::fmt
  )
else()
  target_include_directories(
    ${IRODS_PARSER_NAME}
    PRIVATE
    ${IRODS_EXTERNALS_FULLPATH_FMT}/include
    ${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include
  )
endif()
