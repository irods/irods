#ifndef PARSE_COMMAND_LINE_OPTIONS
#define PARSE_COMMAND_LINE_OPTIONS

#include "parseCommandLine.h"
#include "rodsPath.h"

extern "C"
int parse_opts_and_paths(
    int               _argc,
    char**            _argv,
    rodsArguments_t&  _rods_args,
    rodsEnv*          _rods_env,
    int               _src_type,
    int               _dst_type,
    int               _flag,
    rodsPathInp_t*    _rods_paths );

#endif // PARSE_COMMAND_LINE_OPTIONS
