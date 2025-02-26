# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
It DOES NOT adhere to Semantic Versioning. iRODS 4 servers are backward compatible with iRODS 3.3 servers unless otherwise specified.

## [4.3.4] - 2025-03-XX

This release ...
- deprecations in support of irods 5
- ssl verification
- bug fixes
- clean up

TODO: Consolidate commits

### Changed

[#6043] Remove dynamic_cast_hack
[#6250,#7286] Use system libarchive
[#6319] CMake: include headers in target sources
[#6584] Make `CMAKE_POSITION_INDEPENDENT_CODE` overridable
[#7778] User removal now also removes ACLs
[#7913] Add support for same range of permissions ichmod allows --- atomic acls api
[#7953] Do not log message unless oprType is changed to UNREG_OPR.
[#7993] Remove packedRei from CMakeLists.txt
[#7994] Change hello script to a template
[#8182] Update GenQuery2 flex rule to match whitespace sequences.
[#8200] Add `json_events.hpp` to CMake
[#8201] `parseCommandLine.cpp`: remove unused win32 compatibility code
[#8201] `rodsLog.cpp`: remove unused win32 compatibility code
[#8201] `rodsPath.cpp`: remove unused win32 compatibility code
[#8201] `sockComm.cpp`: remove unused win32 compatibility code
[#7968] Remove centos 7 build from GitHub workflows

# Group these

[#8010] Remove duplicate function declaration
[#8133] Remove duplicate macro definition for GenQuery1.
[#6234] Remove some nonexistent #includes
[#6546] Remove references to functions from `nre.ruleAdminMS.cpp`
[#6546] Remove unused `debug.cpp`
[#6546] Remove unused `nre.ruleAdminMS.cpp`
[#6546] Remove unused `ruleAdmin.cpp`
[#8204] Remove unused `irods_zone_info.cpp`
[#7919] control-plane: remove unused boost.atomic include
[#7942] Remove irods_assert.hpp
[#6421] Federation: Add test to iget with no remote user
[#7412,#7491] Remove test_exception_in_delay_server
[#7722] Add tests for zone name validation.
[#7734] Add tests to ensure iadmin failures exit nonzero
[#7795] iquery: Add test for query tab in where clause
[#7961] Update test for imeta deprecation message
[#7990] Change test so USER_SOCK_CONNECT_TIMEDOUT is also accepted.
[#8046] Connect to server as correct user in rc_switch_user test.
[#8047] Guarantee test users are removed if rc_switch_user test fails.
[#8134,#8135] GenQuery2: Add test for column mappings.
[#8166] Add test for "istream read --count".
[#8182] Add test to guard against whitespace issues in GenQuery2.
[#8183] Add ASCII printable file creation function for testing.
[#8185] Fix memory leaks in GenQuery2 C++ unit test.

[#2600] Doxygen: Add replica_open.h
[#2978] Add more tests changing user type to rodsgroup
[#4984] Clean up headers in rsDataGet and rsDataPut
[#5800] catch boost::bad_lexical_cast by reference
[#6970] clang-format: Allow single-line empty curly braces
[#6972] Silence clang-tidy array decay checks for exception macros
[#7313] Demote clang-tidy checks for magic numbers to warning.
[#7701,#8000] Add documentation for updating the schema version numbers.
[#7751] clang-tidy: demote `cppcoreguidelines-macro-usage` to warning
[#7821] clang-tidy: demote `cppcoreguidelines-avoid-c-arrays` to warning
[#7821] clang-tidy: demote `modernize-avoid-c-arrays` to warning
[#7822] clang-tidy: demote `cppcoreguidelines-pro-type-vararg` to warning
[#7823] Migrate workflows to Ubuntu 24.04

### Removed

THIS SECTION IS ABOUT REMOVAL OF FEATURES

### Deprecated

[#2978] GeneralAdmin: Deprecate "modify" of "user" of type "rodsgroup"
[#6843] Deprecate modification of ACL policy.
[#7608] Deprecate GeneralRowInsert and GeneralRowPurge
[#7835] Deprecate update_deprecated_database_columns.py
[#7977] Deprecate server monitoring microservices
[#7979] Deprecate dataObjLock and dataObjUnlock
[#8042] Deprecate unused global variables in resource manager implementation.
[#8088] Deprecate fillGenQueryInpFromStrCond().
[#8109] Deprecate forkAndExec(...)

### Fixed

[#3581] Avoid SIGABRT by setting pointers to null after deallocation.
[#4984] Return errors on one-sided encryption in parallel transfer
[#6684] Use ProcessType to detect server vs pure client.
[#7599,#7440] Rework logic in \_cllExecSqlNoResult to return proper error codes
[#7685] Fix flex warning for GenQuery2 lexer rules.
[#7722] Validate and reject invalid zone names.
[#7752] Remove `name` from exception
[#7933,#7934] Ignore SIGPIPE so agents do not terminate immediately.
[#7967] Change if statement so ticket use count is only updated on first pass
[#8001] Write client and proxy user info to ips data file in correct order.
[#8017] Modify schema file to enforce required attributes
[#8045] Use std::tolower() with std::transform() correctly.
[#8050] Fix error handling logic for heartbeat operation.
[#8080] Return error on invalid GenQuery1 aggregate function.
[#8095] NREP: write functions return error on nonexistent target
[#8106] Clean up memory in rsDataObjChksum and various endpoints
[#8110] Correct narrowing of floating point values
[#8111] Remove ctor from Cache in NREP to allow for memset
[#8120] Avoid segfault in rcDisconnect
[#8134] GenQuery2: Fix column mappings for user zone.
[#8135] GenQuery2: Use correct table aliases for permissions.
[#8153] Fix nullptr to function argument and unsigned overflow
[#8221] Clear rodsPathInp_t instead of freeing memory.

### Added

[#2978] GeneralAdmin: Add ability to "add" a "group"
[#7947] make checksum buffer size configurable
[#7986] Add JSON-formatted SSL/TLS cert info to MiscSvrInfo API
[#8090] Add Undefined Behavior Sanitizer Option

---

ICOMMANDS

- these are to be included in the icommands changelog
- these will not be included in the server changelog
- server changelog + icommands changelog = release notes for docs.irods.org

[#433,irods/irods#8122] Use more C++ in ienv, ierror, ipwd
[#433] fix ibun help text
[irods/irods#2524,irods/irods#2525] Deprecate ilocate and igetwild
[irods/irods#2600] update irsync help text                                                              
[irods/irods#2978] iadmin: Add support to "add" a "group"
[irods/irods#7286] Remove another reference to externals-provided libarchive
[irods/irods#7286] Remove references to externals-provided libarchive
[irods/irods#7622] Replace examples in iquest help text with link to web page.
[irods/irods#7734] iadmin failures now exit non-zero
[irods/irods#7784] Remove irrelevant help text from iclienthints.
[irods/irods#7838,irods/irods#8122] CMake: remove old centos support code
[irods/irods#7952] Use root resource instead of leaf resource in ichksum help text.
[irods/irods#7959] Deprecate imeta qu.
[irods/irods#7961] Deprecate imeta interactive mode
[irods/irods#7986] Print SSL info in imiscsvrinfo
[irods/irods#8082] itree: Do not display stacktrace on exception
[irods/irods#8088] Ignore deprecation of fillGenQueryInpFromStrCond().
[irods/irods#8090] Add Undefined Behavior Sanitizer Option
[irods/irods#8106] Free memory of leaking objects found in testing
[irods/irods#8120] Remove duplicated rcDisconnect call
[irods/irods#8166] Fix read/write loop in istream.

NOT USED

[#8203] bump version to 4.3.4
[#4984] Fix unencrypted parallel transfer with server encryption enabled
[#4984] Fix unencrypted parallel transfer with server encryption enabled ------ REVERT BY ALAN
