#[=======================================================================[.rst:
ObjectTargetHelpers
-------------------

iRODS makes heavy use of object library targets to group compilation units and
cut down on redundant recompilation. This CMake module includes helper
functions for working with these targets.

.. command:: target_link_objects

  Since object library targets are typically intermediate targets used for
  building executables, shared libraries, and static libraries, they are
  usually not packaged or exported into CMake targets files. Unfortunately,
  this makes passing dependencies down from object library targets tricky.
  Using :command:`target_link_libraries` like normal is problematic when
  generating CMake targets files, as all non-import, non-interface targets
  passed in for linking must be exported along with the linked target.
  Conversely, using the generator expression :genex:`$<TARGET_OBJECTS>` to
  specify the object files as source files for the final target does not pass
  down dependencies from the object library target.

#]=======================================================================]

include_guard(GLOBAL)

cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0051 NEW)

#-----------------------------------------------------------------------------
# Helper function.  DO NOT CALL DIRECTLY.
function(__target_link_objects_impl target linkage objlib is_inner_call)

  # We need to process object library targets in objlib's INTERFACE_LINK_LIBRARIES list and
  # we need to ensure we do not process the same object library target twice. To accomplish
  # this, we add a temporary list property to the target containing all the object library
  # targets processed so far. We also maintain a temporary list property tracking the object
  # library targets currently being processed to detect circular object library target
  # dependencies.

  if (is_inner_call)
    # If this is an inner call, fetch the temporary list properties and return if needed
    get_target_property(objlib_callstack "${target}" __OBJLIB_CALLSTACK)
    get_target_property(processed_objlibs "${target}" __PROCESSED_OBJLIBS)
    if (objlib IN_LIST objlib_callstack OR objlib IN_LIST processed_objlibs)
      return()
    endif()
    unset(processed_objlibs)
  else()
    # Otherwise, initialize objlib callstack and processed objlib lists
    set(objlib_callstack "")
    set_target_properties("${target}" PROPERTIES __OBJLIB_CALLSTACK "${objlib_callstack}")
    set(processed_objlibs "")
    set_target_properties("${target}" PROPERTIES __PROCESSED_OBJLIBS "${processed_objlibs}")
    unset(processed_objlibs)
  endif()
  # add the current objlib to the callstack
  list(APPEND objlib_callstack "${objlib}")
  set_target_properties("${target}" PROPERTIES __OBJLIB_CALLSTACK "${objlib_callstack}")

  if (linkage STREQUAL "INTERFACE")
    # make sure objlib is built first
    add_dependencies(${target} ${objlib})
  else()
    # add object files to source list
    if (CMAKE_VERSION VERSION_LESS "3.21.0")
      # make sure objlib is built first
      # (newer versions of CMake will do this automatically)
      add_dependencies(${target} ${objlib})
    endif()
    target_sources(${target} PRIVATE $<TARGET_OBJECTS:${objlib}>)
  endif()

  # process libraries, pt one
  set(objlib_ilibs)
  get_target_property(objlib_libs "${objlib}" INTERFACE_LINK_LIBRARIES)
  if (objlib_libs)
    foreach (objlib_lib IN LISTS objlib_libs)
      # TODO: handle generator expressions
      if (TARGET "${objlib_lib}")
        get_target_property(ilib_type "${objlib_lib}" TYPE)
        if (ilib_type STREQUAL "OBJECT_LIBRARY")
          __target_link_objects_impl("${target}" "${linkage}" "${objlib_lib}" TRUE)
        else()
          list(APPEND objlib_ilibs "${objlib_lib}")
        endif()
      else()
        list(APPEND objlib_ilibs "${objlib_lib}")
      endif()
    endforeach()
  endif()

  # process everything else
  get_target_property(objlib_includes "${objlib}" INTERFACE_INCLUDE_DIRECTORIES)
  if (objlib_includes)
    foreach (objlib_include IN LISTS objlib_includes)
      target_include_directories("${target}" "${linkage}" "${objlib_include}")
    endforeach()
  endif()
  get_target_property(objlib_sysincludes "${objlib}" INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  if (objlib_sysincludes)
    foreach (objlib_sysinclude IN LISTS objlib_sysincludes)
      target_include_directories("${target}" SYSTEM "${linkage}" "${objlib_sysinclude}")
    endforeach()
  endif()
  if (NOT CMAKE_VERSION VERSION_LESS "3.13.0")
    get_target_property(objlib_linkdirs "${objlib}" INTERFACE_LINK_DIRECTORIES)
    if (objlib_linkdirs)
      foreach (objlib_linkdir IN LISTS objlib_linkdirs)
        target_link_directories("${target}" "${linkage}" "${objlib_linkdir}")
      endforeach()
    endif()
    get_target_property(objlib_linkopts "${objlib}" INTERFACE_LINK_OPTIONS)
    if (objlib_linkopts)
      foreach (objlib_linkopt IN LISTS objlib_linkopts)
        target_link_options("${target}" "${linkage}" "${objlib_linkopt}")
      endforeach()
    endif()
  endif()
  get_target_property(objlib_compdefs "${objlib}" INTERFACE_COMPILE_DEFINITIONS)
  if (objlib_compdefs)
    foreach (objlib_compdef IN LISTS objlib_compdefs)
      target_compile_definitions("${target}" "${linkage}" "${objlib_compdef}")
    endforeach()
  endif()
  get_target_property(objlib_compopts "${objlib}" INTERFACE_COMPILE_OPTIONS)
  if (objlib_compopts)
    foreach (objlib_compopt IN LISTS objlib_compopts)
      target_compile_options("${target}" "${linkage}" "${objlib_compopt}")
    endforeach()
  endif()
  get_target_property(objlib_compfeats "${objlib}" INTERFACE_COMPILE_FEATURES)
  if (objlib_compfeats)
    foreach (objlib_compfeat IN LISTS objlib_compfeats)
      target_compile_features("${target}" "${linkage}" "${objlib_compfeat}")
    endforeach()
  endif()
  if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
    get_target_property(objlib_pchs "${objlib}" INTERFACE_PRECOMPILE_HEADERS)
    if (objlib_pchs)
      foreach (objlib_pch IN LISTS objlib_pchs)
        target_precompile_headers("${target}" "${linkage}" "${objlib_pch}")
      endforeach()
    endif()
  endif()

  # TODO: INTERFACE_POSITION_INDEPENDENT_CODE supports generator expressions, but
  # POSITION_INDEPENDENT_CODE does not.
  get_target_property(objlib_pic "${objlib}" INTERFACE_POSITION_INDEPENDENT_CODE)
  if (DEFINED objlib_pic AND NOT objlib_pic STREQUAL "objlib_pic-NOTFOUND")
    if (NOT linkage STREQUAL "INTERFACE")
      set_target_properties("${target}" PROPERTIES POSITION_INDEPENDENT_CODE "${objlib_pic}")
    endif()
    if (NOT linkage STREQUAL "PRIVATE")
      set_target_properties("${target}" PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE "${objlib_pic}")
    endif()
  endif()
  
  # TODO: INTERFACE_LINK_DEPENDS
  # TODO: INTERFACE_SOURCES
  

  # process libraries, pt two
  foreach (objlib_lib IN LISTS objlib_ilibs)
    target_link_libraries("${target}" "${linkage}" "${objlib_lib}")
  endforeach()

  ## some sanity checks before we clean up
  # ensure callstack is the same as it was when we added the current objlib
  get_target_property(objlib_callstack_2 "${target}" __OBJLIB_CALLSTACK)
  if (NOT objlib_callstack STREQUAL objlib_callstack_2)
    message(FATAL_ERROR "objlib callstack consistency check failed (list equality)\n")
  endif()
  unset(objlib_callstack_2)
  # ensure the current objlib is the last item on the callstack
  list(GET objlib_callstack -1 last_callstack_objlib)
  if (NOT last_callstack_objlib STREQUAL objlib)
    message(FATAL_ERROR "objlib callstack consistency check failed (last entry)\n")
  endif()
  unset(last_callstack_objlib)

  ## cleanup time
  # add current objlib to processed list
  set_property(TARGET "${target}" APPEND PROPERTY __PROCESSED_OBJLIBS "${objlib}")
  # remove current objlib from callstack
  list(REMOVE_AT objlib_callstack -1)
  set_target_properties("${target}" PROPERTIES __OBJLIB_CALLSTACK "${objlib_callstack}")
  if (NOT is_inner_call)
    # one last sanity check for the outermost call - ensure callstack is empty
    if (NOT objlib_callstack STREQUAL "")
      message(FATAL_ERROR "objlib callstack consistency check failed (cleanup)\n")
    endif()
    # clear temporary list properties
    set_property(TARGET "${target}" PROPERTY __PROCESSED_OBJLIBS)
    set_property(TARGET "${target}" PROPERTY __OBJLIB_CALLSTACK)
  endif()

endfunction()

if (NOT CMAKE_DUMMY_SOURCE_FILES_DIR)
  set(CMAKE_DUMMY_SOURCE_FILES_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/DummyFiles")
  set(CMAKE_DUMMY_SOURCE_FILES_DIR "${CMAKE_DUMMY_SOURCE_FILES_DIR}" CACHE INTERNAL "Directory containing dummy source files")
endif()

if (NOT TARGET cmake_dummy_source_files_directory)
  add_custom_command(
    OUTPUT "${CMAKE_DUMMY_SOURCE_FILES_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_DUMMY_SOURCE_FILES_DIR}"
  )
  add_custom_target(
    cmake_dummy_source_files_directory
    DEPENDS "${CMAKE_DUMMY_SOURCE_FILES_DIR}"
  )
endif()

function(create_dummy_source_file_target language)
  if(language STREQUAL "C")
    set(dummy_ext ".c")
  elseif(language STREQUAL "CXX")
    set(dummy_ext ".cpp")
  else()
    message(FATAL_ERROR "Unknown language:\n  ${language}\nSupported languages: C, CXX.\n")
  endif()

  if (NOT CMAKE_${language}_DUMMY_SOURCE_FILE)
    set(CMAKE_${language}_DUMMY_SOURCE_FILE "${CMAKE_DUMMY_SOURCE_FILES_DIR}/empty_dummy${dummy_ext}")
    set(CMAKE_${language}_DUMMY_SOURCE_FILE "${CMAKE_${language}_DUMMY_SOURCE_FILE}" PARENT_SCOPE)
    set(CMAKE_${language}_DUMMY_SOURCE_FILE "${CMAKE_${language}_DUMMY_SOURCE_FILE}" CACHE INTERNAL "Dummy ${language} source file")
  endif()

  if (NOT TARGET cmake_dummy_${language}_source_file)
    add_custom_command(
      OUTPUT "${CMAKE_${language}_DUMMY_SOURCE_FILE}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_${language}_DUMMY_SOURCE_FILE}"
      MAIN_DEPENDENCY "${CMAKE_DUMMY_SOURCE_FILES_DIR}"
    )
    add_custom_target(
      cmake_dummy_${language}_source_file
      DEPENDS "${CMAKE_${language}_DUMMY_SOURCE_FILE}"
    )
  endif()
endfunction()

function(target_link_objects target)
  get_target_property(target_type ${target} TYPE)
  if (target_type STREQUAL "OBJECT_LIBRARY")
    set(target_is_objlib TRUE)
  else()
    set(target_is_objlib FALSE)
  endif()

  set(current_linkage "PUBLIC")
  foreach (arg ${ARGN})
    if ("x${arg}" MATCHES "^x(PRIVATE|PUBLIC|INTERFACE)$")
      set(current_linkage "${arg}")
    elseif (NOT TARGET "${arg}")
      message(FATAL_ERROR "Unknown argument:\n  ${arg}\n")
    else()
    get_target_property(objlib_type "${arg}" TYPE)
    if (NOT objlib_type STREQUAL "OBJECT_LIBRARY")
      message(FATAL_ERROR "Non-objlib target passed to target_link_objects:\n  ${objlib}\n")
    endif()
      if (target_is_objlib)
        target_link_libraries("${target}" "${current_linkage}" "${arg}")
      else()
        __target_link_objects_impl("${target}" "${current_linkage}" "${arg}" FALSE)
      endif()
    endif()
  endforeach()
endfunction()

cmake_policy(POP)