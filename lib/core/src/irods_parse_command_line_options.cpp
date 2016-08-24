#include "getRodsEnv.h"
#include "irods_parse_command_line_options.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "rodsErrorTable.h"
namespace fs = boost::filesystem;

#include <vector>
#include <string>
#include <iostream>

typedef std::vector< std::string > path_list_t;

// this is global due to the fact that the storage of the
// various strings used by _rods_args is managed by the
// lifetime of this map - only used by icommand client execs
static boost::program_options::variables_map global_prog_ops_var_map;

// currently only written to support iput.  needs to be expanded to
// support the full range of icommand options
static int parse_program_options(
    int              _argc,
    char**           _argv,
    rodsArguments_t& _rods_args,
    path_list_t&     _paths ) {
    namespace po = boost::program_options;

    po::options_description opt_desc( "options" );
    opt_desc.add_options()
    ( "help,h", "show command usage" )
    ( "all,a", "all - update all existing copies" )
    ( "bulk,b", "bulk upload to reduce overhead" )
    ( "force,f", "force - write data-object even it exists already; overwrite it" )
    ( "redirect,I", "redirect connection - redirect the connection to connect directly to the resource server." )
    ( "checksum,k", "checksum - calculate a checksum on the data server-side, and store it in the catalog" )
    ( "verify_checksum,K", "verify checksum - calculate and verify the checksum on the data, both client-side and server-side, without storing in the catalog." )
    ( "repl_num,n", po::value<std::string>(), "replNum  - the replica to be replaced, typically not needed" )
    ( "num_threads,N", po::value<int>(), "numThreads - the number of threads to use for the transfer. A value of 0 means no threading. By default (-N option not used) the server decides the number of threads to use." )
    ( "physical_path,p", po::value<std::string>(), "physicalPath - the absolute physical path of the uploaded file on the server" )
    ( "progress,P", "output the progress of the upload." )
    ( "rbudp,Q", "use RBUDP (datagram) protocol for the data transfer" )
    ( "recursive,r", "recursive - store the whole subdirectory" )
    ( "dest_resc,R", po::value<std::string>(), "resource - specifies the resource to store to. This can also be specified in your environment or via a rule set up by the administrator" )
    ( "ticket,t", po::value<std::string>(), "ticket - ticket (string) to use for ticket-based access" )
    ( "renew_socket,T", "renew socket connection after 10 minutes" )
    ( "verbose,v", "verbose" )
    ( "very_verbose,V", "very verbose" )
    ( "data_type,D", po::value<std::string>(), "dataType - the data type string" )
    ( "restart_file,X", po::value<std::string>(), "restartFile - specifies that the restart option is on and the restartFile input specifies a local file that contains the restart information." )
    ( "link", "ignore symlink." )
    ( "lfrestart", po::value<std::string>(), "lfRestartFile - specifies that the large file restart option is on and the lfRestartFile input specifies a local file that contains the restart information." )
    ( "retries", po::value<int>(), "count - Retry the iput in case of error. The 'count' input specifies the number of times to retry. It must be used with the -X option" )
    ( "wlock", "use advisory write (exclusive) lock for the upload" )
    ( "rlock", "use advisory read lock for the download" )
    ( "purgec", "Purge the staged cache copy after uploading an object to a" )
    ( "kv_pass", po::value<std::string>(), "pass key-value strings through to the plugin infrastructure" )
    ( "metadata", po::value<std::string>(), "atomically assign metadata after a data object is put" )
    ( "acl", po::value<std::string>(), "atomically apply an access control list after a data object is put" )
    ( "path_args", po::value<path_list_t>( &_paths )->composing(), "some files and stuffs" );

    po::positional_options_description pos_desc;
    pos_desc.add( "path_args", -1 );

    try {
        po::store(
            po::command_line_parser(
                _argc, _argv ).options(
                opt_desc ).positional(
                pos_desc ).run(), global_prog_ops_var_map );
        po::notify( global_prog_ops_var_map );
    }
    catch ( po::error& _e ) {
        std::cout << std::endl
                  << "Error: "
                  << _e.what()
                  << std::endl
                  << std::endl;
        return SYS_INVALID_INPUT_PARAM;

    }

    bool have_verify = global_prog_ops_var_map.count( "verify_checksum" ) > 0;
    bool have_stdout = _paths.end() !=
                       std::find(
                           _paths.begin(),
                           _paths.end(),
                           std::string( STDOUT_FILE_NAME ) );
    if ( have_verify && have_stdout ) {
        std::cerr << "Cannot verify checksum if data is piped to stdout." << std::endl << std::endl;
        return SYS_INVALID_INPUT_PARAM;
    }

    memset( &_rods_args, 0, sizeof( _rods_args ) );
    if ( global_prog_ops_var_map.count( "help" ) ) {
        _rods_args.help = 1;
        return 0;
    }
    if ( global_prog_ops_var_map.count( "all" ) ) {
        _rods_args.all = 1;
    }
    if ( global_prog_ops_var_map.count( "bulk" ) ) {
        _rods_args.bulk = 1;
    }
    if ( global_prog_ops_var_map.count( "force" ) ) {
        _rods_args.bulk = 1;
    }
    if ( global_prog_ops_var_map.count( "force" ) ) {
        _rods_args.force = 1;
    }
    if ( global_prog_ops_var_map.count( "redirect" ) ) {
        _rods_args.redirectConn = 1;
    }
    if ( global_prog_ops_var_map.count( "checksum" ) ) {
        _rods_args.checksum = 1;
    }
    if ( global_prog_ops_var_map.count( "verify_checksum" ) ) {
        _rods_args.verifyChecksum = 1;
    }
    if ( global_prog_ops_var_map.count( "" ) ) {
        _rods_args.verifyChecksum = 1;
    }
    if ( global_prog_ops_var_map.count( "repl_num" ) ) {
        _rods_args.replNum = 1;
        try {
            _rods_args.replNumValue = ( char* )global_prog_ops_var_map[ "repl_num" ].as<std::string>().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "num_threads" ) ) {
        _rods_args.number = 1;
        try {
            _rods_args.numberValue = global_prog_ops_var_map[ "num_threads" ].as<int>();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "physical_path" ) ) {
        _rods_args.physicalPath = 1;
        try {
            _rods_args.physicalPathString = ( char* )global_prog_ops_var_map[ "physical_path" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "progress" ) ) {
        _rods_args.progressFlag = 1;
    }
    if ( global_prog_ops_var_map.count( "rbudp" ) ) {
        _rods_args.rbudp = 1;
    }
    if ( global_prog_ops_var_map.count( "recursive" ) ) {
        _rods_args.recursive = 1;
    }
    if ( global_prog_ops_var_map.count( "dest_resc" ) ) {
        _rods_args.resource = 1;
        try {
            _rods_args.resourceString = ( char* )global_prog_ops_var_map[ "dest_resc" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "ticket" ) ) {
        _rods_args.ticket = 1;
        try {
            _rods_args.ticketString = ( char* )global_prog_ops_var_map[ "ticket" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "renew_socket" ) ) {
        _rods_args.reconnect = 1;
    }
    if ( global_prog_ops_var_map.count( "verbose" ) ) {
        _rods_args.verbose = 1;
    }
    if ( global_prog_ops_var_map.count( "very_verbose" ) ) {
        _rods_args.verbose = 1;
        _rods_args.veryVerbose = 1;
        rodsLogLevel( LOG_NOTICE );
    }
    if ( global_prog_ops_var_map.count( "data_type" ) ) {
        _rods_args.dataType = 1;
        try {
            _rods_args.dataTypeString = ( char* )global_prog_ops_var_map[ "data_type" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "restart_file" ) ) {
        _rods_args.restart = 1;
        try {
            _rods_args.restartFileString = ( char* )global_prog_ops_var_map[ "restart_file" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "link" ) ) {
        _rods_args.link = 1;
    }
    if ( global_prog_ops_var_map.count( "lfrestart" ) ) {
        _rods_args.lfrestart = 1;
        try {
            _rods_args.lfrestartFileString = ( char* )global_prog_ops_var_map[ "lfrestart" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "retries" ) ) {
        _rods_args.retries = 1;
        try {
            _rods_args.retriesValue = global_prog_ops_var_map[ "retries" ].as< int >();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "wlock" ) ) {
        _rods_args.wlock = 1;
    }
    if ( global_prog_ops_var_map.count( "rlock" ) ) {
        _rods_args.rlock = 1;
    }
    if ( global_prog_ops_var_map.count( "purgec" ) ) {
        _rods_args.purgeCache = 1;
    }
    if ( global_prog_ops_var_map.count( "kv_pass" ) ) {
        _rods_args.kv_pass = 1;
        try {
            _rods_args.kv_pass_string = ( char* )global_prog_ops_var_map[ "kv_pass" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "metadata" ) ) {
        try {
            _rods_args.metadata_string = ( char* )global_prog_ops_var_map[ "metadata" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }
    if ( global_prog_ops_var_map.count( "acl" ) ) {
        try {
            _rods_args.acl_string = ( char* )global_prog_ops_var_map[ "acl" ].as< std::string >().c_str();
        }
        catch ( const boost::bad_any_cast& ) {
            return INVALID_ANY_CAST;
        }
    }

    return 0;

} // parse_program_options

static int build_irods_path_structure(
    const path_list_t& _path_list,
    rodsEnv*           _rods_env,
    int                _src_type,
    int                _dst_type,
    int                _flag,
    rodsPathInp_t*     _rods_paths ) {

    int numSrc = 0;

    if ( _rods_paths == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( _rods_paths, 0, sizeof( rodsPathInp_t ) );

    if ( _path_list.size() <= 0 ) {
        if ( ( _flag & ALLOW_NO_SRC_FLAG ) == 0 ) {
            return USER__NULL_INPUT_ERR;
        }
        else {
            numSrc = 1;
        }
    }
    else if ( _path_list.size() == 1 ) {
        numSrc = 1;
    }
    else if ( _dst_type == NO_INPUT_T ) {            /* no dest input */
        numSrc = _path_list.size();
    }
    else {
        numSrc = _path_list.size() - 1;
    }

    int status = 0;
    for ( int i = 0; i < numSrc; i++ ) {
        if ( _path_list.size() <= 0 ) {
            /* just add cwd */
            addSrcInPath( _rods_paths, "." );
        }
        else {
            addSrcInPath( _rods_paths, _path_list[ i ].c_str() );
        }
        if ( _src_type <= COLL_OBJ_T ) {
            status = parseRodsPath( &_rods_paths->srcPath[i], _rods_env );
        }
        else {
            status = parseLocalPath( &_rods_paths->srcPath[i] );
        }
        if ( status < 0 ) {
            return status;
        }
    }

    if ( _dst_type != NO_INPUT_T ) {
        _rods_paths->destPath = ( rodsPath_t* )malloc( sizeof( rodsPath_t ) );
        memset( _rods_paths->destPath, 0, sizeof( rodsPath_t ) );
        if ( _path_list.size() > 1 ) {
            rstrcpy(
                _rods_paths->destPath->inPath,
                _path_list.rbegin()->c_str(),
                MAX_NAME_LEN );
        }
        else {
            rstrcpy( _rods_paths->destPath->inPath, ".", MAX_NAME_LEN );
        }

        if ( _dst_type <= COLL_OBJ_T ) {
            status = parseRodsPath( _rods_paths->destPath, _rods_env );
        }
        else if ( strcmp( _rods_paths->destPath->inPath, STDOUT_FILE_NAME ) == 0 ) {
            snprintf( _rods_paths->destPath->outPath, sizeof( _rods_paths->destPath->outPath ), "%s", STDOUT_FILE_NAME );
            _rods_paths->destPath->objType = UNKNOWN_FILE_T;
            _rods_paths->destPath->objState = NOT_EXIST_ST;
            status = NOT_EXIST_ST;
        }
        else {
            status = parseLocalPath( _rods_paths->destPath );
        }
    }

    return status;

} // build_irods_path_structure

void set_sp_option( const char* _exec ) {
    if( !_exec ) {
        return;
    }

    fs::path p( _exec );
    setenv( SP_OPTION, p.filename().c_str(), 1 );

} // set_sp_option

// single C-friendly interface for the above two functions
int parse_opts_and_paths(
    int               _argc,
    char**            _argv,
    rodsArguments_t&  _rods_args,
    rodsEnv*          _rods_env,
    int               _src_type,
    int               _dst_type,
    int               _flag,
    rodsPathInp_t*    _rods_paths ) {
    if( !_argv || !_argv[0] ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    set_sp_option( _argv[0] );

    path_list_t paths;
    int p_err = parse_program_options(
                    _argc,
                    _argv,
                    _rods_args,
                    paths );
    if ( p_err < 0 ) {
        return p_err;

    }

    if ( _rods_args.help ) {
        return 0;
    }

    p_err = build_irods_path_structure(
                paths,
                _rods_env,
                _src_type,
                _dst_type,
                _flag,
                _rods_paths );
    if ( p_err < 0 ) {
        return p_err;
    }

    return 0;

} // parse_opts_and_paths
