#ifndef IRODS_AGENT_GLOBALS_HPP
#define IRODS_AGENT_GLOBALS_HPP

// Globals aren't the best, but given that C++17 supports initialization
// of global variables across translation units without violating ODR, it
// is okay to define globals for the agent factory here.
//
// With that said, let's keep this to a minimum. Defining global variables
// in this file should be a last resort.

#include <cstdint>
#include <csignal>

// This global is the flag which allows agents to react to stop instructions
// from the agent factory.
//
// It is set by signal handlers defined by the agent factory and MUST NOT
// be used for anything else.
//
// Low-level systems which need to react to stop instructions should include
// this file and check this flag.
inline volatile std::sig_atomic_t g_terminate = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// This global is the flag which allows agents to react to graceful stop
// instructions from the agent factory.
//
// It is set by signal handlers defined by the agent factory and MUST NOT
// be used for anything else.
//
// Low-level systems which need to react to stop instructions should include
// this file and check this flag.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline volatile std::sig_atomic_t g_terminate_graceful = 0;

// This global is the flag which allows the agents to react to invalid
// grid configuration values for access_time.resolution_in_seconds.
//
// This global variable is meant to give rsDataObjOpen a way to know when
// to produce log noise due to a bad value.
//
// Use of this global variable keeps the agents from needing to rely on
// server_properties (for speed). Only the agent factory should modify
// this variable.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline bool g_atime_invalid_resolution_in_seconds_detected = false;

// This global stores the final value that is to be used for access time
// resolution.
//
// It is initialized with the default value for resolution_in_seconds.
//
// Use of this global variable keeps the agents from needing to rely on
// server_properties (for speed). Only the agent factory should modify
// this variable.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline std::int32_t g_atime_resolution_in_seconds = 86400;

#endif // IRODS_AGENT_GLOBALS_HPP
