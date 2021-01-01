#   CHECK_CPP_MACRO_DEFINITION(MACRO_NAME VARIABLE [LANGUAGE <language>]
#
# Check if the macro exists and determine its size.  On return,
# "HAVE_${VARIABLE}" holds the existence of the macro, and "${VARIABLE}"
# holds its definition.
# Both ``HAVE_${VARIABLE}`` and ``${VARIABLE}`` will be created as internal
# cache variables.
#
# If LANGUAGE is set, the specified compiler will be used to perform the
# check. Acceptable values are C and CXX
#
# The following variables may be set before calling this macro to modify
# the way the check is run:
#
# ::
#
#   CMAKE_REQUIRED_FLAGS = string of compile command line flags
#   CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#   CMAKE_REQUIRED_INCLUDES = list of include directories
#   CMAKE_REQUIRED_LIBRARIES = list of libraries to link
#   CMAKE_REQUIRED_QUIET = execute quietly without messages
#   CMAKE_EXTRA_INCLUDE_FILES = list of extra headers to include
#   CMAKE_REQUIRED_CMAKE_FLAGS = list of flags to pass to CMake

get_filename_component(__check_cpp_macro_def_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)

include_guard(GLOBAL)

cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)
#cmake_policy(SET CMP0056 NEW)

#-----------------------------------------------------------------------------
# Helper function.  DO NOT CALL DIRECTLY.
function(__check_cpp_macro_def_impl macro var language)
  if(NOT CMAKE_REQUIRED_QUIET)
    message(STATUS "Check definition of ${langauge} preprocessor macro ${macro}")
  endif()

  # Include header files.
  set(headers)
  foreach(h ${CMAKE_EXTRA_INCLUDE_FILES})
    string(APPEND headers "#include \"${h}\"\n")
  endforeach()

  # Perform the check.

  if(language STREQUAL "C")
    set(src ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckCPPMacroDefinition/${var}.c)
  elseif(language STREQUAL "CXX")
    set(src ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckCPPMacroDefinition/${var}.cpp)
  else()
    message(FATAL_ERROR "Unknown language:\n  ${language}\nSupported languages: C, CXX.\n")
  endif()
  set(bin ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckCPPMacroDefinition/${var}${CMAKE_EXECUTABLE_SUFFIX_${language}})
  configure_file(${__check_cpp_macro_def_dir}/CheckCPPMacroDefinition.c.in ${src} @ONLY)
  try_compile(HAVE_${var} ${CMAKE_BINARY_DIR} ${src}
    COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
    LINK_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES}
    CMAKE_FLAGS
      ${CMAKE_REQUIRED_CMAKE_FLAGS}
      "-DCOMPILE_DEFINITIONS:STRING=${CMAKE_REQUIRED_FLAGS}"
      "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}"
    OUTPUT_VARIABLE output
    COPY_FILE ${bin}
    )

  if(HAVE_${var})
    # The check compiled. Hooray! Run it and hope for the best.
    execute_process(COMMAND "${bin}"
      RESULT_VARIABLE run_result
      OUTPUT_VARIABLE ${var})
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Check definition of ${langauge} preprocessor macro ${macro} - done")
    endif()
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining definition of ${langauge} preprocessor macro ${macro} passed with the following compile output:\n${output}\n\n")
    set(${var} "${${var}}" CACHE INTERNAL "CHECK_CPP_MACRO_DEFINITION: CMAKE_TOSTRING(${macro})")
  else()
    # The check failed to compile.
    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Check definition of ${langauge} preprocessor macro ${macro} - failed")
    endif()
    file(READ ${src} content)
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Determining definition of ${langauge} preprocessor macro ${macro} passed with the following compile output:\n${output}\n${src}:\n${content}\n\n")
    set(${var} "" CACHE INTERNAL "CHECK_CPP_MACRO_DEFINITION: ${macro} undefined")
  endif()
endfunction()

#-----------------------------------------------------------------------------
macro(CHECK_CPP_MACRO_DEFINITION MACRO VARIABLE)
  # parse arguments
  unset(doing)
  foreach(arg ${ARGN})
    if("x${arg}" STREQUAL "xLANGUAGE") # change to MATCHES for more keys
      set(doing "${arg}")
      set(_CHECK_CPP_MACRO_DEFINITION_${doing} "")
    elseif("x${doing}" STREQUAL "xLANGUAGE")
      set(_CHECK_CPP_MACRO_DEFINITION_${doing} "${arg}")
      unset(doing)
    else()
      message(FATAL_ERROR "Unknown argument:\n  ${arg}\n")
    endif()
  endforeach()
  if("x${doing}" MATCHES "^x(LANGUAGE)$")
    message(FATAL_ERROR "Missing argument:\n  ${doing} arguments requires a value\n")
  endif()
  if(DEFINED _CHECK_CPP_MACRO_DEFINITION_LANGUAGE)
    if(NOT "x${_CHECK_CPP_MACRO_DEFINITION_LANGUAGE}" MATCHES "^x(C|CXX)$")
      message(FATAL_ERROR "Unknown language:\n  ${_CHECK_CPP_MACRO_DEFINITION_LANGUAGE}.\nSupported languages: C, CXX.\n")
    endif()
    set(_language ${_CHECK_CPP_MACRO_DEFINITION_LANGUAGE})
  else()
    set(_language C)
  endif()
  unset(_CHECK_CPP_MACRO_DEFINITION_LANGUAGE)

  if(NOT DEFINED HAVE_${VARIABLE})
    __check_cpp_macro_def_impl(${MACRO} ${VARIABLE} ${_language})
  endif()

endmacro()

#-----------------------------------------------------------------------------
cmake_policy(POP)
