#ifndef IRODS_AGENT_GLOBALS_HPP
#define IRODS_AGENT_GLOBALS_HPP

// Globals aren't the best, but given that C++17 supports initialization
// of global variable across translation units without violating ODR, it
// is okay to define globals for the agent factory here.
//
// With that said, let's keep this to a minimum. Defining global variables
// in this file should be a last resort.

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
inline volatile std::sig_atomic_t g_terminate_graceful = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#endif // IRODS_AGENT_GLOBALS_HPP
